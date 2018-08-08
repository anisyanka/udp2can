
#include "logger.h"
#include <stdio.h>



void logger_debug  (char *format, ... ) {

  printf("DEBUG: ");

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  return;
}      

void logger_info (char *format, ... ) {

  printf("INFO: ");

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  
  return;
}      

void logger_error  (char *format, ... ) {

  printf("ERROR: ");
  
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  
  return;
}      


void logger_hexdump(char *data, int len) {

  return;
}