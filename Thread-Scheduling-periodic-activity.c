/*
Liam Gleeson
Assignment 2
CSCI 247
*/

// Initializing the preprocessor directives needed for the assignment
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>
#include <signal.h>
#include <errno.h>
#include <signal.h>

// Initialzing the macro deifnitions needed for the assignment
#define	MAX_THREAD_COUNT		3
#define LOW_THREAD_PRIORITY		50
#define STACK_SIZE				0x400000
#define	MAX_TASK_COUNT			5

//define error handling to check for errors throughout the code
#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

// mutex's initialized
pthread_mutex_t		g_DisplayMutex = PTHREAD_MUTEX_INITIALIZER;;
pthread_mutex_t 	g_SignalMutex = PTHREAD_MUTEX_INITIALIZER;
//
typedef struct{
	int			threadCount;
	pthread_t	threadId;
	int		threadPolicy;
	int			threadPri;
  struct sched_param param;
	long		processTime;
	//long		timeStamp[MAX_TASK_COUNT+1];
	int64_t		timeStamp[MAX_TASK_COUNT+1];
	time_t		startTime;
	time_t		endTime;
	int sig;
	sigset_t alarm_sig;
	int wakeups_missed;
  int signal_number;
  sigset_t timer_signal;
  int missed_signal_count;
	timer_t timer_id;
	int timer_Period;
} ThreadArgs;

ThreadArgs g_ThreadArgs[MAX_THREAD_COUNT];

void InitThreadArgs(void) //initialize thread args so that they can get displayed
{
	for(int i=0;i<MAX_THREAD_COUNT;i++)
	{
		g_ThreadArgs[i].threadCount = 0;
		g_ThreadArgs[i].threadId =0;
		g_ThreadArgs[i].threadPri = 0;
		g_ThreadArgs[i].processTime =0;
		for(int y=0; y<MAX_TASK_COUNT+1; y++)
		{
			g_ThreadArgs[i].timeStamp[y] = 0;
		}
	}
	pthread_mutex_init ( &g_DisplayMutex, NULL);
}

void DisplayThreadSchdAttributes( void )  //displays the thread schedule attributes such as schedule type and policy.
{
	   int policy;
	   struct sched_param param;
	   pthread_getschedparam(pthread_self(), &policy, &param);
	   printf("DisplayThreadSchdAttributes:\n        threadID = 0x%lx\n        policy = %s\n        priority = %d\n",
			pthread_self(),
		   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
		   (policy == SCHED_RR)	   ? "SCHED_RR" :
		   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
		   "???",
		   param.sched_priority);
}

void DisplayThreadArgs(ThreadArgs*	myThreadArg) //displays the thread argument details such as start and stop time
{
  char buffer[90];
	pthread_mutex_lock(&g_DisplayMutex);
	if( myThreadArg )
	{
	//	printf("DisplayThreadArgs: Thread Id [%lx] Pri [%d] ProcTime [%ld]\n",
	//	myThreadArg->threadId, myThreadArg->threadPri, myThreadArg->processTime);
			DisplayThreadSchdAttributes();
			printf("        startTime = %s        endTime = %s\n", ctime(&myThreadArg->startTime), ctime_r(&myThreadArg->endTime, buffer));
			printf("        TimeStamp [%"PRId64"]\n", myThreadArg->timeStamp[0] );
			for(int y=1; y<MAX_TASK_COUNT+1; y++)
			{
				printf("        TimeStamp [%"PRId64"]   Delta [%"PRId64"]us     Jitter[%"PRId64"]us\n", myThreadArg->timeStamp[y],
							(myThreadArg->timeStamp[y]-myThreadArg->timeStamp[y-1]),
							(myThreadArg->timeStamp[y]-myThreadArg->timeStamp[y-1]-myThreadArg->timer_Period));
			}
	}
	else
	{
		for(int i=0;i<MAX_THREAD_COUNT;i++)
		{
			//printf("DisplayThreadArgs: Thread Id [%lx] Pri [%d] ProcTime [%ld]\n",
			//	g_ThreadArgs[i].threadId, g_ThreadArgs[i].threadPri, g_ThreadArgs[i].processTime);
			DisplayThreadSchdAttributes();
			printf("   TimeStamp (task starting) 	[%ld]\n", g_ThreadArgs[i].timeStamp[0] );

			for(int y=1; y<MAX_TASK_COUNT+1; y++)
			{
				printf("   TimeStamp (task completed)	[%ld]   Delta [%ld]\n", g_ThreadArgs[i].timeStamp[y],
							(g_ThreadArgs[i].timeStamp[y]-g_ThreadArgs[i].timeStamp[y-1]));
			}
		}
	}
	pthread_mutex_unlock	(&g_DisplayMutex);
}

//create the createandarmtimer function that will be called from any thread to wait for the timer created by this thread.
int CreateAndArmTimer(int unsigned period, ThreadArgs* info)
{
   int ret;
   unsigned int nanoseconds;
   unsigned int seconds;
   struct sigevent mySignalEvent;
   struct itimerspec timerSpec;

   //initialize static int to have a unique value that increments up as the sigrtmin does also,
   //Use thread mutex lock and unlock to make sure thread is safe.
   pthread_mutex_lock(&g_SignalMutex);
   static int sigvar = 0;
    if (sigvar == 0) {
      sigvar = SIGRTMIN;
    }
      sigvar++;


   info->sig = sigvar;
   pthread_mutex_unlock(&g_SignalMutex);

   mySignalEvent.sigev_notify = SIGEV_SIGNAL;
   mySignalEvent.sigev_signo = info->sig; //A value b/t SIGRTMIN and SIGRTMAX
   mySignalEvent.sigev_value.sival_ptr = (void *)&(info->timer_id);
   //use the handle_error_en function to check if all function calls are working properly
   // Initialize local variable "mySignalEvent" and Call "timer_create" to create timer
   ret = timer_create (CLOCK_MONOTONIC, &mySignalEvent,&info->timer_id);
   if (ret != 0) { //check to see if timer create function is the cause of an error
     handle_error_en(ret, "timer create has an error");
   }
   // Initialize info->timer_Period to the argument period
   info->timer_Period = period;
   // Use "sigemptyset" to initialize info->alarm_sig
   ret = sigemptyset(&info->alarm_sig);
   if (ret != 0) {
     handle_error_en(ret, "sig empty set has an error");
   }
   //Use "signaddset" to add info->sig to info->alarm_sig
   ret = sigaddset(&info->alarm_sig, info->sig);
   if (ret != 0) {
     handle_error_en(ret, "sig add set has an error");
   }
   seconds= period/1000000;
   nanoseconds= (period -(seconds * 1000000)) * 1000;
   timerSpec.it_interval.tv_sec = seconds;
   timerSpec.it_interval.tv_nsec = nanoseconds;
   timerSpec.it_value.tv_sec = seconds;
   timerSpec.it_value.tv_nsec = nanoseconds;
   //Initialize local variable "timerSpec" and call "timer_settime" to set the time out
   ret = timer_settime (info ->timer_id, 0, & timerSpec, NULL);
   if (ret != 0) {
     handle_error_en(ret, "timer set time has an error");
   }
}
// initialize wait function to wait for the timer created by this thread
static void wait_period (ThreadArgs *info)
{
	int sig;
	// Call "sigwait" to wait on info->alarm_sig
  sig = sigwait(&info->alarm_sig, &info->sig);
  if (sig != 0){
    handle_error_en(sig,"the sigwait has an error");
  }
	// Update "info->wakeups_missed" with the return from "timer_getoverrun"
  info->wakeups_missed += timer_getoverrun(info->timer_id);
  sig = timer_getoverrun(info->timer_id);
  if (sig == -1) {
    handle_error_en(sig, "timer get over run has an error");
  }
}

//the main for the threads
void* threadFunction(void *arg)
{
	ThreadArgs*	myThreadArg;
  int err;
	struct timeval	t1;
	struct timespec tms;
	int y, retVal;
	myThreadArg = (ThreadArgs*)arg;
	if( myThreadArg->threadId != pthread_self() )
	{
		printf("mismatched thread Ids... exiting...\n");
		pthread_exit(arg);
	}
	else
	{
		retVal = pthread_setschedparam(pthread_self(), myThreadArg->threadPolicy, &myThreadArg->param);		//SCHED_FIFO, SCHED_RR, SCHED_OTHER
		if(retVal != 0){
			handle_error_en(retVal, "pthread_setschedparam");
		}
		myThreadArg->processTime = 0;
	}
	CreateAndArmTimer(myThreadArg->timer_Period, myThreadArg); //calls the CreateAndArmTimer function
	myThreadArg->startTime = time(NULL);
	//printf("\n [%d]starttime %s\n",myThreadArg->threadCount, ctime(&myThreadArg->startTime));
  y=0;
  clock_gettime(CLOCK_REALTIME, &tms); //gives the timestamp prior to running
  myThreadArg->timeStamp[y] = tms.tv_sec *1000000;
  myThreadArg->timeStamp[y] += tms.tv_nsec/1000;
  if(tms.tv_nsec % 1000 >= 500 )  myThreadArg->timeStamp[y]++;
	while (y < MAX_TASK_COUNT)
	{
		gettimeofday(&t1, NULL);
		wait_period(myThreadArg);
		y++;
		err = clock_gettime(CLOCK_REALTIME, &tms);
    if (err == -1) {
      handle_error_en(err, "clock get time error");
    }
		myThreadArg->timeStamp[y] = tms.tv_sec *1000000;
		myThreadArg->timeStamp[y] += tms.tv_nsec/1000;
		if(tms.tv_nsec % 1000 >= 500 )  myThreadArg->timeStamp[y]++;
	}
	time_t tmp;
	tmp = time(NULL);
	myThreadArg->endTime = time(NULL);
	DisplayThreadArgs(myThreadArg);
	pthread_exit(NULL);
	return NULL;
}

//the main that initializes it all
int main (int argc, char *argv[])
{
	int threadCount = 0;
  int err, i;
	int fifoPri = 60;
	int period = 1;
	int retVal;

	pthread_attr_t threadAttrib;

   sigset_t timer_signal;
   sigemptyset(&timer_signal);
   for (int i = SIGRTMIN; i <=SIGRTMAX; i++)
    err = sigaddset(&timer_signal, i);
    if (err == -1) {
      handle_error_en(err, "sig add set error in main");
    }
   err = sigprocmask(SIG_BLOCK, &timer_signal, NULL);
   if (err == -1) {
     handle_error_en(err, "sig proc mask error in main ");
   }
	InitThreadArgs();

   for (int i = 0; i < MAX_THREAD_COUNT; i ++)
   {
	  g_ThreadArgs[i].threadCount = i+1;
      g_ThreadArgs[i].threadPolicy = SCHED_FIFO;
      g_ThreadArgs[i].param.sched_priority = fifoPri++;
	  g_ThreadArgs[i].timer_Period = (period << i)*1000000;
      retVal = pthread_create(&g_ThreadArgs[i].threadId, NULL, threadFunction, &g_ThreadArgs[i]);
	  if(retVal != 0)
	  {
		handle_error_en(retVal, "pthread_create");
	  }
   }
	for(int i = 0; i < MAX_THREAD_COUNT; i++)
	{
      pthread_join(g_ThreadArgs[i].threadId, NULL);
    }
	printf("Main thread is exiting\n");
    return 0;
}

/* Jitter analysis
The jitter is the difference between the time the thread completed its goal and the time it was told to do so. For the three threads, each which are meant
to be respectively at 1,000,000 , 2,000,000 , and 4,000,000, the jitter refers to the difference between what the threads delta is meant
to be and what it ends up being. For example, 1,000,562 means the jitter is 562 and that means it was 562 microseconds off from its expected delta.
A negative jitter just means the thread completed earlier than expected, as seen also from the the delta being less than 1,000,0000 , 2,000,000 , or 4,000,000.
So a jitter of -91 means the thread completed 91 microseconds faster than expected.
*/
