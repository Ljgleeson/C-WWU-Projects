#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

pthread_mutex_t mutex1;
int life_jackets = 10;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int wait_line_length = 0;
int * done_flag;
int counter = 0;

struct node {
  int data;
  int jacket;
  struct node *next;
};


struct queue {
  struct node *head;
  struct node *tail;
};

struct queue theQueue;

void queue_init (struct queue *queue) {
  queue->head = NULL;
  queue->tail = NULL;
}

bool queue_isEmpty (struct queue *queue) {
  return queue->head == NULL;
}

//added the jacket value to each thread in queue rather than creating the struct with that value
void queue_insert (struct queue* queue, int value, int jacket) {
  struct node *tmp = malloc(sizeof(struct node));
  if (tmp == NULL) {
    fputs ("malloc failed\n", stderr);
    exit(1);
  }

  // create the node 
  tmp->data = value;
  tmp->next = NULL;
  tmp->jacket = jacket;

  if (queue->head == NULL) {
    queue->head = tmp;
  } else {
    queue->tail->next = tmp;
  }
  queue->tail = tmp;
}

int queue_remove ( struct queue *queue ) {
  int retval = 0;
  struct node *tmp;
  
  if (!queue_isEmpty(queue)) {
    tmp = queue->head;
    retval = tmp->data;
    queue->head = tmp->next;
    free(tmp);
  }
  return retval;
}

void fatal (long n) {
  printf ("Fatal error, lock or unlock error, thread %ld.\n", n);
  exit(n);
}

int getRandNum(int lower, int upper){
  int num = (rand() % (upper - lower + 1)) + lower;
  return num;
}

void getJackets(long threadn, int jacket){
    
  //"If there are less than 5 groups, add group to wait queue"
  if(wait_line_length < 5){
    queue_insert(&theQueue, threadn, jacket);
    printf("Group: %ld, is now waiting in queue. \n", threadn);
    wait_line_length ++;
    while(jacket > life_jackets){
      pthread_cond_wait(&cond, &mutex1);
    }
  }
  //"If 5 or more groups waiting, report group will not wait and exit thread"
  else{
    pthread_mutex_unlock(&mutex1);
    printf("Line is too long, Group %ld will not wait! \n", threadn);
    counter++;
    done_flag[threadn] = 1;
    pthread_exit(NULL);      
  }
}


void putJackets(long threadn, int jacket, int usage_time){
  pthread_mutex_unlock(&mutex1);
  life_jackets = life_jackets - jacket;
  printf("Group: %ld, is now using %d jackets. %d life jackets remain \n", threadn, jacket, life_jackets);

  //"call sleep with the use time for this group"
  sleep(usage_time); 
  done_flag[threadn] = 1;
  //"Report the return of jackets and number of life jackets available"
  life_jackets = life_jackets + jacket;
  printf("Group: %ld, has returned with their %d jackets. %d life jackets remain \n", threadn, jacket, life_jackets);
}



//start routine:
void * thread_body ( void *arg ) {
    
  long threadn = (long) arg;
  long local_data = random() % 100000;
  int boat = getRandNum(0,2);
  int jacket;

  //"Print out group number, water craft requested, and number of life jackets needed"
  if(boat == 0){
    char boat_name[] = "kayak";
    jacket = 1;
    printf("Group: %ld. Boat type: %s. Life Jackets required: %d\n", threadn, boat_name, jacket);
  }else if (boat == 1){
    char boat_name[] = "Canoe";
    jacket = 2;
    printf("Group: %ld. Boat type: %s. Life Jackets required: %d\n", threadn, boat_name, jacket);
  }else{
    char boat_name[] = "Sailboat";
    jacket = 4;
    printf("Group: %ld. Boat type: %s. Life Jackets required: %d\n", threadn, boat_name, jacket);
  }

  int usage_time = getRandNum(0,7);
  pthread_mutex_lock(&mutex1);
   
  //"check to see if there are enough life jackets"
  if(jacket > life_jackets){
    getJackets(threadn, jacket);
  }

  //"when enoguh jackets available, report group will be using them and remaining life jackets"
  if(jacket <= life_jackets){
    putJackets(threadn, jacket, usage_time);
  }
  
  //"If theres a queue: unblock as many as possible without running out of life jackets."     
  while((queue_isEmpty(&theQueue) == false) && theQueue.head->jacket <= life_jackets){
    queue_remove(&theQueue);
    pthread_cond_signal(&cond); //restarts the head of queue thread that was waiting on the condition cond. 
    wait_line_length --;   
  }
  
  pthread_exit(NULL);
}




int main (int argc, char ** argv) {
  int group_count;
  int arrival_rate;
  
  if (argc == 2){    //If only amount of groups listed
    group_count = atoi(argv[1]);
    arrival_rate = getRandNum(0, 6);
    srandom(0);
  }
  else if (argc == 3){ //amount of groups + new group arrival rate
    group_count = atoi(argv[1]);
    arrival_rate = getRandNum(0, atoi(argv[2]));
    srandom(0);
  }
  else if (argc == 4){ //Initalizing the random number generator + other 2
    group_count = atoi(argv[1]);
    arrival_rate = getRandNum(0, atoi(argv[2]));
    srandom(atoi(argv[3]));
  }
  else{
    printf("Too many arguments for this sorry \n");
    return -1;
  }

  //create a flag and have it set each threads flag
  done_flag = malloc(sizeof(*done_flag) * group_count);
  for(int i = 0; i < group_count; i++){
    done_flag[i] = 0;
  }
  
  //creates group_count amount of threads
   pthread_t ids[group_count];  
   int err;
   long i = 0;
   long final_data = 0;

   pthread_mutex_init(&mutex1, NULL);
   int thread_count = 0;

   //loop that creates all threads and joins the threads that have finished
   while(1){

     //if check that will create each thread at the start and wont create any more afterwards
     if (thread_count < group_count){      
       //start a new group
       sleep(arrival_rate);
       
       //creating a thread by passing it into the thread_body
       err = pthread_create (&ids[i], NULL, thread_body, (void *)i);
       //error check
       if (err) {
	 fprintf (stderr, "Can't create thread %ld\n", i);
	 exit (1);
       }
       thread_count++;
     }
     
     void *retval;
    
     //"Main function should join all remaining threads so all threads run to termination"
     //for loop since as we need to search through each thread, not just current one as some down the line can terminate before the current one
     for(int x = 0; x < group_count; x++){
       if(done_flag[x] == 1){
	 done_flag[x] = 2; //assigning a different value so doesnt go through the loop again if already terminated.
	 counter ++;
	 pthread_join(ids[x], &retval);
       }
     }
     
     //breaks the loop once every thread has terminated
     if(counter == group_count){
       break;
     }
     
     i++;    
   } //end of while loop
   

   pthread_mutex_destroy(&mutex1);  // Not needed, but here for completeness
   return 0;
}

