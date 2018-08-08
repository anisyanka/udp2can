#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "cJSON.h"
#include "logger.h"
#include "interface_mapping.h"


/** pointer to table of interfaces. Table is fall in when 'intfmap_read_mapping_table' function is running
    and then it read only **/
struct if_map_t * interface_map = NULL;



void intfmap_read_mapping_table (void) {

  /** open mapping file **/
  FILE * map_file;
  map_file = fopen("interface_map.json", "r");
  if (map_file == NULL) {

    logger_error("Can't open input file 'interface_map.json'!\n");
    exit(EXIT_FAILURE);
  }

  /** find out file size and allocate buffer for its data**/
  struct stat statistics;
  if ( (stat("interface_map.json", &statistics)) < 0 ) {

    perror("stat(map_file, &statistics)");
    exit(EXIT_FAILURE);
  }

  /** set limit on file size **/
  if (statistics.st_size > MAX_INTERFACE_MAP_SIZE) {

    logger_error("too large mapping file!\n");
    exit(EXIT_FAILURE);
  }

  /** **/
  char * buffer = (char *)malloc(statistics.st_size + 1);

  /** read all data from file to buffer and close the file **/
  fread((void *)buffer, 1, statistics.st_size, map_file);
  fclose(map_file);
  buffer[statistics.st_size] = '\0';

  /** start JSON parsing **/
  cJSON * json = cJSON_Parse(buffer);
  if (json == NULL) {

    cJSON_Delete(json);
    free(buffer);
    logger_error("cjson_parse. Bad file format!\n");
    exit(EXIT_FAILURE);
  }

  int i;
  int num_interfaces = cJSON_GetArraySize(json);

  cJSON *ip2can = NULL;
  cJSON *port2can = NULL;
  cJSON *can2ip = NULL;
  cJSON *can2port = NULL;
  cJSON *can_id_interface = NULL;
  cJSON *can_id_protocol = NULL;
  cJSON *tmp_json = NULL;

  struct if_map_t *tmp_itnerface_map = NULL;

  for (i = 0; i < num_interfaces; ++i) {

    if (i == 0)  {
      interface_map = (struct if_map_t *)malloc(sizeof(struct if_map_t));
      tmp_itnerface_map = interface_map;
    }
    else {
      tmp_itnerface_map = tmp_itnerface_map->next = (struct if_map_t *)malloc(sizeof(struct if_map_t));
    }

    // distinguish one of the some objects
    tmp_json = cJSON_GetArrayItem(json, i);

    ip2can = cJSON_GetObjectItem(tmp_json, "ip-to-can");
    port2can = cJSON_GetObjectItem(tmp_json, "port-to-can");
    can2ip = cJSON_GetObjectItem(tmp_json, "can-to-ip");
    can2port = cJSON_GetObjectItem(tmp_json, "can-to-port");
    can_id_interface = cJSON_GetObjectItem(tmp_json, "can-interface-id");
    can_id_protocol = cJSON_GetObjectItem(tmp_json, "can-protocol-id");

    if ( (ip2can == NULL) && (port2can == NULL) && (can2ip == NULL) && \
         (can2port == NULL) && (can_id_interface == NULL) && (can_id_protocol) ) {

      cJSON_Delete(json);
      free(buffer);
      logger_error("wrong JSON\n");
      exit(EXIT_FAILURE);
    }

    // check valid ip address to send data to can
    if ( (cJSON_IsString(ip2can)) && (ip2can->valuestring != NULL) ) {

      tmp_itnerface_map->to_can.ip = ip2can->valuestring;
      logger_info("ip-->can = %s\n", ip2can->valuestring);
    }
    else {

      cJSON_Delete(json);
      free(buffer);
      logger_error("ip port must be in string format\n");
      exit(EXIT_FAILURE);
    }

    // check valid ip port to send data to can
    if (cJSON_IsNumber(port2can)) {

      tmp_itnerface_map->to_can.port = port2can->valueint;
      logger_info("port-->can = %d\n", port2can->valueint);
    }
    else {

      cJSON_Delete(json);
      free(buffer);
      logger_error("port must be in integer format\n");
      exit(EXIT_FAILURE);
    }

    // check valid ip address to send data from can
    if ( (cJSON_IsString(can2ip)) && (can2ip->valuestring != NULL) ) {

      tmp_itnerface_map->from_can.ip = can2ip->valuestring;
      logger_info("can-->ip = %s\n", can2ip->valuestring);
    }
    else {

      cJSON_Delete(json);
      free(buffer);
      logger_error("ip port must be in string format\n");
      exit(EXIT_FAILURE);
    }

    // check valid ip port to sen data from can
    if (cJSON_IsNumber(can2port)) {

      tmp_itnerface_map->from_can.port = can2port->valueint;
      logger_info("can-->port = %d\n", can2port->valueint);
    }
    else {

      cJSON_Delete(json);
      free(buffer);
      logger_error("port must be in integer format\n");
      exit(EXIT_FAILURE);
    }

    // check can-id-interface
    if ( (cJSON_IsString(can_id_interface)) && (can_id_interface->valuestring != NULL) ) {
      
      tmp_itnerface_map->to_can.can_id_interface = can_id_interface->valuestring;
      tmp_itnerface_map->from_can.can_id_interface = can_id_interface->valuestring;
      logger_info("can_id_interface = %s\n", can_id_interface->valuestring);
    }
    else {

      cJSON_Delete(json);
      free(buffer);
      logger_error("can_id_interface must be in string format\n");
      exit(EXIT_FAILURE);    
    }

    // check can-id-protocol
    if (cJSON_IsNumber(can_id_protocol)) {

      tmp_itnerface_map->from_can.can_id_protocol = can_id_protocol->valueint;
      tmp_itnerface_map->to_can.can_id_protocol = can_id_protocol->valueint;
      logger_info("can_id_protocol = %d\n\n", can_id_protocol->valueint);
    }
    else {

      cJSON_Delete(json);
      free(buffer);
      logger_error("can_id_protocol must be in integer format\n");
      exit(EXIT_FAILURE);
    }
  }
  tmp_itnerface_map->next = NULL;

  /** end parsing **/
  free(buffer);
  return;
}
/*******/


int intfmap_get_diff_udp2can_conn (void) {
  
  int num_udp2can_conn = 0;

  struct if_map_t *tmp_itnerface_tbl = NULL;
  struct if_map_t *tmp_itnerface_tbl1 = NULL;
  struct if_map_t *tmp_itnerface_tbl2 = NULL;

  /** calculate num of udp2can different connections and delete same connections **/
  tmp_itnerface_tbl = interface_map;

  if (tmp_itnerface_tbl->next == NULL) {

    num_udp2can_conn = 1;
  }
  else {

    num_udp2can_conn = 1;

    while (tmp_itnerface_tbl->next != NULL) {

      tmp_itnerface_tbl1 = tmp_itnerface_tbl;
      tmp_itnerface_tbl2 = tmp_itnerface_tbl->next;

      while (tmp_itnerface_tbl2 != NULL) {

        // compare string ip-port-can from tbl1 vs ip-port-can tbl2
        // if strings are same - delete one same connection

        // update for next iteration
        tmp_itnerface_tbl2 = tmp_itnerface_tbl2->next;
      }
      
      // update for next iteration
      tmp_itnerface_tbl = tmp_itnerface_tbl->next;
    }
  }

  return num_udp2can_conn;
}
/*******/


int intfmap_get_diff_can2udp_conn (void) {

  static int num_can2udp_conn = 0;

  struct if_map_t *tmp_itnerface_tbl1 = NULL;
  struct if_map_t *tmp_itnerface_tbl2 = NULL;

  /** calculate num of can2udp different connection connections and delete same connections **/


  return num_can2udp_conn;
}
/*******/

/*
    char str[MAX_STRLEN_FOR_INTERFACE_NAMING];
    char str_port[10];

    strcpy(str, interface_map->to_can.ip);
    strcat(str,":");
    strcat(str, itoa(interface_map->to_can.port, str_port, 10));
    strcat(str, "->");
    strcat(str, interface_map->to_can.can_id_interface);*/