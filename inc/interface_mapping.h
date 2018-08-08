
#ifndef INTERFACE_MAPPING_H
#define INTERFACE_MAPPING_H


#include <stdint.h>
#include <unistd.h>

/** max size of file with interface mapping in bytes **/
#define MAX_INTERFACE_MAP_SIZE    ((int)2048)

/** struct need to define mapping table between interfaces **/
struct if_map_t
{
  struct if_map_t *next;
  
	struct
	{
		char               *ip;
    char               *can_id_interface;
    int                can_id_protocol;
  	unsigned short int port;
	}to_can;

	struct
	{
		char               *ip;
    char               *can_id_interface;
    int                can_id_protocol;
  	unsigned short int port;
	}from_can;
};








/**  **/
void intfmap_read_mapping_table     (void);
int  intfmap_get_diff_udp2can_conn  (void);
int  intfmap_get_diff_can2udp_conn  (void);






#endif /*INTERFACE_MAPPING_H*/
