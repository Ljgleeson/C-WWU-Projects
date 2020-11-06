#ifndef shm_h
#define shm_h

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "shm.h"
//<Define an enum called StatusEnus with the enumerations "INVALID", "VALID" and "CONSUMED">
typedef enum {INVALID, VALID, CONSUMED} StatusEnum;
//<Define a typedef structure with the enum above and an "int" variable called "data">
typedef struct{
  int data;
  StatusEnum status;
} ShmData;

#endif
