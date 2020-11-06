#include "shm.h"

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


int main(int argc, char* argv[])
{
  int retVal = 0;
  ShmData* shmPtr;
  //Opens a POSIX shared memory object
  int object = shm_open("/shm.h", O_RDWR, 0666);
  if (object == -1) { //returns -1 on failure
    printf("Tried running before server passes argument \n");
    exit(0);
  }
  //<Use the "mmap" API to memory map the file descriptor>
  shmPtr = mmap(0,sizeof(ShmData), PROT_READ | PROT_WRITE, MAP_SHARED, object, 0 );
  if (shmPtr == MAP_FAILED) {
     shm_unlink("shm.h");
     handle_error_en(shmPtr, "Error in mmap");
  }

  printf("[Client]: Waiting for valid data ...\n");
  while(shmPtr->status != VALID)
    {
      sleep(1);
    }
  printf("[Client]: Received %d\n",shmPtr->data);
  shmPtr->status = CONSUMED;
  // <use the "munmap" API to unmap the pointer>
  int addr = munmap(shmPtr, sizeof(addr));
  if (addr == -1) { //returns -1 on failure
    shm_unlink("shm.h");
		handle_error_en(addr, "Error in munmap");
  	//printf("munmap has failed");
  }
  printf("[Client]: Client exiting...\n");

  return(retVal);

}
