#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdarg.h>


void logger_debug  	(char *format, ... );
void logger_info 	(char *format, ... );
void logger_error  	(char *format, ... );
void logger_empty   (char *format, ... );
void logger_todo 	(char *format, ... ); 


/**   **/
void logger_hexdump(char *data, int len);


#endif /*LOGGER_H*/
