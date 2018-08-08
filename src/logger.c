
#include "logger.h"
#include <stdio.h>



void logger_print_help (void) {

  printf("Help: \n");
  return;
}
/*******/


void logger_print_debug (const char * s) {

  printf("DEBUG: %s\n", s);
  return;
}
/*******/


void logger_print_info (const char * s) {

  printf("INFO: %s\n", s);
  return;
}
/*******/


void logger_print_error (const char * s) {

  printf("ERROR: %s\n", s);
  return;
}
/*******/

