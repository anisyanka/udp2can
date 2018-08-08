/* 
 *  Test case for Linux-embedded developers:
 *  UDP-to-CAN converter
 *  
 *  Author: Anisimov Alexander
 *  E-mail: anisimov.alexander.s@gmail.com
 *
 *  HOW IT WORKS:
 *  
 *
 *
 *
 *
 */
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <arpa/inet.h>


/** pointer to table of interfaces. Table is fall in when read_mapping_table function and then only read in threads **/
extern struct if_map_t * interface_map;


/**  **/
struct conn_data_t{

  char           *ip;
  char           *can_interface_id;
  int             can_protocol_id;
  unsigned short port;
};


/** existed threads **/
void *udp2can_listener (void * args);
void *can2udp_listener (void * args);



int main (const int const argc, const char const *argv[]) {

  int i;

  /** make interface map table ( struct 'interface_map') from "interface_map.json" file **/
  intfmap_read_mapping_table();

  /** calculate different number of udp->can and can->udp connections  **/
  const int num_diff_udp2can_conn = intfmap_get_diff_udp2can_conn();
  const int num_diff_can2udp_conn = intfmap_get_diff_can2udp_conn();

  #if (UDP2CAN_DEBUG)
    printf("num_diff_udp2can_conn = %d\n", (int)num_diff_udp2can_conn);
    printf("num_diff_can2udp_conn = %d\n", (int)num_diff_can2udp_conn);
  #endif

  /** create one udp thread per one can connection **/
  pthread_t udp2can_pthreads[num_diff_udp2can_conn];
  struct conn_data_t udp2can_conn_data[num_diff_udp2can_conn];

  for (i = 0; i < num_diff_udp2can_conn; ++i) {

    udp2can_conn_data[i].ip = interface_map->to_can.ip;
    udp2can_conn_data[i].port = interface_map->to_can.port;
    udp2can_conn_data[i].can_interface_id = interface_map->to_can.can_id_interface;
    udp2can_conn_data[i].can_protocol_id = interface_map->to_can.can_id_protocol;

    if ( (pthread_create( &udp2can_pthreads[i], NULL,
                          udp2can_listener, (void *)&udp2can_conn_data[i])) != 0 )  {

      perror("udp-to-can pthread_create()");
      exit(EXIT_FAILURE);
    }
  }

  /** create one can thread per one udp connection **/
  pthread_t can2udp_pthreads[num_diff_can2udp_conn];
  struct conn_data_t can2udp_conn_data[num_diff_can2udp_conn];

  for (i = 0; i < num_diff_can2udp_conn; ++i) {

    can2udp_conn_data[i].ip = interface_map->from_can.ip;
    can2udp_conn_data[i].port = interface_map->from_can.port;
    can2udp_conn_data[i].can_interface_id = interface_map->from_can.can_id_interface;
    can2udp_conn_data[i].can_protocol_id = interface_map->from_can.can_id_protocol;
    
    if ( (pthread_create( &can2udp_pthreads[i], NULL,
                          can2udp_listener, (void *)&can2udp_conn_data[i])) != 0 )  {

      perror("can-to-udp pthread_create()");
      exit(EXIT_FAILURE);
    } 
  }

  while (true);

  return EXIT_SUCCESS;
}
/*************/





void *udp2can_listener (void * args) {

  int i;  
  struct conn_data_t  * conn_data = (struct conn_data_t *)args;

  /** open UDP socket **/
  int num_udp_byte = 0;
  char input_udp_data[MAX_LEN_CAN_PACKET+1]={0};

  int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_socket < 0) {

    perror("Error while opening udp-socket in udp2can_listener");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(conn_data->port);
  addr.sin_addr.s_addr = inet_addr(conn_data->ip);

  if ( bind(udp_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {

    perror("Error while udp-socket bind in udp2can_listener");
    close(udp_socket);
    exit(EXIT_FAILURE);
  }

  /** open CAN socket **/
  int can_socket;
  struct sockaddr_can can_addr;
  struct can_frame frame;
  struct ifreq ifr;
  const char *ifname = conn_data->can_interface_id;

  if((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {

    perror("Error while opening can-socket in udp2can_listener");
    close(udp_socket);
    close(can_socket);
    exit(EXIT_FAILURE);
  }

  strcpy(ifr.ifr_name, ifname);
  ioctl(can_socket, SIOCGIFINDEX, &ifr);

  can_addr.can_family  = AF_CAN;
  can_addr.can_ifindex = ifr.ifr_ifindex;
  
  if ( bind(can_socket, (struct sockaddr *)&can_addr, sizeof(can_addr)) < 0 ) {
    
    perror("Error while can-socket bind in udp2can_listener");
    close(udp_socket);
    close(can_socket);
    
    /* MY  PC has no any can interface.That's why bind can socket returns error */
    exit(EXIT_FAILURE);
  }

  /** listenning UPD port and rewrite this data to CAN socket **/
  while (true) {

    #if (UDP2CAN_DEBUG)
      printf("Wait new data from udp socket . . .\n");
    #endif

    num_udp_byte = recvfrom(udp_socket, input_udp_data, MAX_LEN_CAN_PACKET, 0,NULL, NULL);

    #if (UDP2CAN_DEBUG)
      input_udp_data[num_udp_byte] = '\0';
      printf("New UDP data from %s:%d received \n", conn_data->ip, conn_data->port);
      printf("%s\n", input_udp_data);
    #endif

    frame.can_id  = conn_data->can_protocol_id;
    frame.can_dlc = num_udp_byte;

    for (i = 0; i < num_udp_byte; ++i)
      frame.data[i] = input_udp_data[i];
    
    // my be  take mutex
    write(can_socket, &frame, sizeof(struct can_frame));
    // my be release mutex

  }
}
/*************/



void *can2udp_listener (void * args) {
  
  int i;  
  struct conn_data_t  * conn_data = (struct conn_data_t *)args;
  int num_can_byte = 0;
  char can_data[MAX_LEN_CAN_PACKET+1]={0};

  /** open UDP socket **/
  int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_socket < 0) {

    perror("Error while opening udp-socket in can2udp_listener");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(conn_data->port);
  addr.sin_addr.s_addr = inet_addr(conn_data->ip);

  if ( bind(udp_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {

    perror("Error while udp-socket bind in can2udp_listener");
    close(udp_socket);
    exit(EXIT_FAILURE);
  }

  /** open CAN socket **/
  int can_socket;
  struct sockaddr_can can_addr;
  struct can_frame frame;
  struct ifreq ifr;
  const char *ifname = conn_data->can_interface_id;

  if((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {

    perror("Error while opening can-socket in can2udp_listener");
    close(udp_socket);
    close(can_socket);
    exit(EXIT_FAILURE);
  }

  strcpy(ifr.ifr_name, ifname);
  ioctl(can_socket, SIOCGIFINDEX, &ifr);

  can_addr.can_family  = AF_CAN;
  can_addr.can_ifindex = ifr.ifr_ifindex;
  
  if ( bind(can_socket, (struct sockaddr *)&can_addr, sizeof(can_addr)) < 0 ) {
    
    perror("Error while can-socket bind in can2udp_listener");
    close(udp_socket);
    close(can_socket);   
    
    /* MY  PC has no any can interface.That's why bind can socket returns error */
    exit(EXIT_FAILURE);
  }

  /** listenning CAN socket and rewrite this data to UDP socket **/
  while (true) {

    #if (UDP2CAN_DEBUG)
      printf("Wait new data from can socket . . .\n");
    #endif

    num_can_byte = read(can_socket, can_data, sizeof (can_data));\

    #if (UDP2CAN_DEBUG)
      can_data[num_can_byte] = '\0';
      printf("New CAN data from %s received \n", conn_data->can_interface_id);
      printf("%s\n", can_data);
    #endif

    // my be  take mutex
    sendto(udp_socket, can_data, num_can_byte, 0, NULL,  sizeof(addr));
    // my be release mutex

  }
}
/*************/
