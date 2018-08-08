
#ifndef INTERFACE_MAPPING_H
#define INTERFACE_MAPPING_H

#include "main.h"
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

/** max size of file with interface mapping in bytes **/
#define MAX_INTERFACE_MAP_SIZE    ((int)2048)

/**  **/
#define MAX_STRLEN_FOR_INTERFACE_NAMING ((int)128)

/** struct need to define mapping table between interfaces **/
struct if_map_t {

  struct if_map_t *next;
  
	struct  {

		char               *ip;
    char               *can_id_interface;
    int                can_id_protocol;
  	unsigned short int port;

    // string in format ip:port->can
    // need for define same interfaces with help strcmp() function
    char               beauty_str_format[MAX_STRLEN_FOR_INTERFACE_NAMING];
    bool               is_need_mutex;
	} to_can;

	struct {

		char               *ip;
    char               *can_id_interface;
    int                can_id_protocol;
  	unsigned short int port;

    // string in format can->ip:port
    // need for define same interfaces with help strcmp() function
    char               beauty_str_format[MAX_STRLEN_FOR_INTERFACE_NAMING];
    bool               is_need_mutex;
	} from_can;
};








/**  **/
void intfmap_read_mapping_table     (void);
int  intfmap_get_diff_udp2can_conn  (void);
int  intfmap_get_diff_can2udp_conn  (void);






#endif /*INTERFACE_MAPPING_H*/
