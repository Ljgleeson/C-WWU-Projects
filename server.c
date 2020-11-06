#include "shm.h"

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char* argv[])
{
	int retVal = 0;
  // shm, and from that all of the type def structures, is a pointer to shmPtr.
	ShmData* shmPtr;
  int max = 2147483647;
  long flow = atol(argv[1]);
  if (flow > max) {
    printf("Integer passed is an overflow. \n" );
    exit(0);
  }
  int x = 0;
  if (argv[1][0] == '-') {
    x++;
  }
  while (x < strlen(argv[1]))  {
     if (!isdigit(argv[1][x])) {
      printf("Argument passed needs to be an integer\n");
      exit(0);
    }
   x++;
 }
  // Checks to make sure there is only two arguments in your argc, ./server and your value.
	if (argc != 2) { //  <Confirm argc is 2 and if not print a usage string.>
		printf("usage: server <int data> \n");
		return 0;
	}
//creates and opens a new (or an existing) POSIX shared memory object. A POSIX shared memory is in effect a handle which can be used by unrelated processes
//to mmap the same region of shared memory.
	int object = shm_open("/shm.h", O_CREAT | O_RDWR, 0666);
	if (object == -1) { //returns -1 on failure
		handle_error_en(object, "Error in creating object");
	}
	// Sets the size of the shared memory object.
	retVal = ftruncate(object, sizeof(ShmData));
	if (retVal == -1) { //takes the object created and the size of our typedef struct to set the size of the shared memory.
		shm_unlink("shm.h");
		handle_error_en(retVal,"Error in ftruncate");
	}
	// Map the shared memory object into the virtual address space of the calling process.
	shmPtr = mmap(0,sizeof(ShmData), PROT_READ | PROT_WRITE, MAP_SHARED, object, 0 );
	//have the starting address be null causing the kernel to choose the address, take the size of our typedef, give it the desired memory protection of the mapping,
	//have the flag set to MAP_SHARED to shareupdates to the mapping of other processes. take the object created and finally set the offset to 0.
	if (shmPtr == MAP_FAILED) { // the value MAP_FAILED ((void*) - 1) will return on failure
		 close(object);
		 shm_unlink("shm.h");
		 handle_error_en(shmPtr, "Error in mmap");
	//	 printf("error in mmap \n");
	//	 return 0;
	}
  	//	<Set the "status" field to INVALID>
	shmPtr->status = INVALID;
  	//	<Set the "data" field to atoi(argv[1])>
    // check over flow for this argv meaning value is higher than max integer
	shmPtr->data = atoi(argv[1]);
  	//	<Set the "status" field to VALID>
	shmPtr->status = VALID;
  //while loop that waits until the client has recieved the passed argument
  printf("[Server]: Server data Valid... waiting for client\n");
  while(shmPtr->status != CONSUMED)
    {
      sleep(1);
    }
  printf("[Server]: Server Data consumed!\n");
  // Server data is consumed so now time to use the munmap system call to delete the mappings for the specified address range and stops further references.
	//The region is also automatically unmapped when the process is terminated.
	int addr = munmap(shmPtr, sizeof(addr));
	if (addr == -1) { //returns -1 on failure
		close(object);
		shm_unlink("shm.h");
		handle_error_en(addr, "Error in munmap");
	}
	// closes a file descriptor so that it no longer refers to any file and may be reused.
	int clo = close(object);
	if (clo == -1) { //returns -1 on failure
		shm_unlink("shm.h");
		handle_error_en(object, "Error in close");
	}
	// Removes a shared memory object name and once all processes have unmapped the object, it de-allocates and destroys the contents of the associated memory region.
	int unlink = shm_unlink("shm.h");
	if (unlink == -1) { //returns -1 on failure
		handle_error_en(unlink,"Error in unlink");
	}
  printf("[Server]: Server exiting...\n");
  return(retVal);
}
/* IPC analysis:
Inter-process communication could be used to get periodic data from the producer at regular intervals by having a clock timer run in both a producers function and a consumers function.
If our server code was all in one function in the producers program, you could have the producer run this function every n-seconds, using the clock timer, to constantly try to update
the data that will be sent to the client. If the consumer has not recieved the data yet, the producer gets held up until the client recieved it. This means that the consumer will always
receive the data whenever it calls the client function. So adding a clock timer in the consumer function at a longer time interval than the producers would allow the consumer function
to receive the data from the producer at regular intervals, only depending on the time interval set by the consumer.

*/
