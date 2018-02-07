/******************************************************************************
 ******************************************************************************

							 CSCI3120 - OPERATING SYSTEMS
			  ASSIGNMENT #3  - A MULTITHREADED SCHEDULING WEB SERVER
				       			
   	 					 	    STEPHEN SAMPSON
					         	    B00568374
						  JULY 2016 - DALHOUSIE UNIVERSITY

 ******************************************************************************
 ******************************************************************************/

	/* NOTE: I have chosen to continue building on my own code rather than
	 * to use the provided solution. It just seems much more fun this way.
	 *	
    * I have adapted my linked list implementation and thread creation from
	 * sources found online but the resources are referenced. They are also
	 * not a direct copy this note is simply in case others have used the same
	 * resources and our code looks similar because of it.
	 */

/******************************************************************************
 *                              INTIALIZATION                                 *
 ******************************************************************************/

/* Header files used in this program  */
#include <pthread.h>
#include <stdbool.h>
#include "universal_functions.h"
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "network.h"
#include "extern.h"
#include "worker.h"

/* Function Prototypes  */
int request_port(void);
int validate_port(int requested_port);
int request_scheduler(void);
int determine_scheduler(char *desired_scheduler);
int ask_threads(void);
int request_cache(void);
	
/******************************************************************************
 *                    				MAIN FUNCTION                                *
 ******************************************************************************/

int main(int argc, char *argv[]) {
	/* SET UP AND INITIALIZE VARIABLES */
	int loopcount = 0;										// loop count
	int port_number = 0; 									// port to initialize network
	int scheduler = -1;										// Scheduler Mode
	int num_wthreads = 0;									// # worker threads to create
	int client_socket = -1;									// socket connected on
	int cache_size = 0;
	char *strscheduler;										// scheduler name
	char cwd[256];					    						// SWS current working dir
	mode = -1;													// initialize scheduler mode
	quant1 = 0;													//	initialize q1 quantum
	quant2 = 0;													// initialize q2 quantum
	quant3 = 0;													// initialize q3 quantum
	sequence = 0;												// initialize seq counter
	debug = 0;													// initialize debug mode
	cnt = 0;														// initialize jobs in process	
	
	/* CONFIGURE PROGRAM BEHAVIOR BASED ON COMMAND LINE ARGS  */
	if(argc==1){										 		// no args on cmd line
		port_number = request_port();				 		// request a port 
		num_wthreads = ask_threads();						// request #worker threads
		cache_size = request_cache();						// request cache size
		scheduler = request_scheduler();			 		// request a scheduler
		}	
	if(argc==2){
		port_number = atoi(argv[1]);						// port declared on cmd line
		num_wthreads = ask_threads();						// request #worker threads
		cache_size = request_cache();						// request cache size
		scheduler = request_scheduler(); 				// request a scheduler
	}
	if(argc==3){
		port_number = atoi(argv[1]);						// port declared on cmd line 
		num_wthreads = ask_threads();						// request #worker threads
		scheduler = determine_scheduler(argv[2]); 	// interpret scheduler
		cache_size = request_cache();						// request cache size
	}
	if(argc==4){
		port_number = atoi(argv[1]);						// port declared on cmd line 
		scheduler = determine_scheduler(argv[2]);		// interpret scheduler 
		num_wthreads = atoi(argv[3]);						// threads to create 
		cache_size = request_cache();						// request cache size
	}
	if(argc==5){
		port_number = atoi(argv[1]);						// port declared on cmd line 
		scheduler = determine_scheduler(argv[2]);		// interpret scheduler 
		num_wthreads = atoi(argv[3]);						// threads to create 
		cache_size = atoi(argv[4]);						// size of cache to create
	}
	if(argc==6) {
		port_number = atoi(argv[1]);						// port declared on cmd line
		scheduler = determine_scheduler(argv[2]);		//	interpret scheduler 
		num_wthreads = atoi(argv[3]);						//	threads to create 
		cache_size = atoi(argv[4]);
		if (strcmp(argv[5],"-d")==0){;					// verify debug mode
			debug = 1;							 				// set debug 
			printf("\nDebugging Enabled\n");
			printf("Program is running from: %s\n", getcwd(cwd, sizeof(cwd)));
		}
	}

	/* FOR DEBUGGING ONLY, PRINT NAME OF SCHEDULER RATHER THAN JUST INT MODE */
	if(scheduler==0){
		strscheduler = "Shortest Job First";
		mode = 0;
	} else if (scheduler==1){
		strscheduler = "Round Robin";
		mode = 1;
	} else if (scheduler==2) {
		strscheduler = "Multi Level Queue with Feedback";
		mode = 2;
	}
	if(debug==1)
		printf("Scheduler Mode: %s\n", strscheduler);
	
	network_init(port_number);	// initialize network on specified port
	set_quantums(mode);
	cache_init(cache_size);
	/* CREATE SPECIFIED NUMBER OF THREADS */
	/* NOTE: Thread creation adapted from 
	 * https://computing.llnl.gov/tutorials/pthreads/ */
	pthread_t threads[num_wthreads];
	int retcode = NULL;
	long created = NULL;
	for(created=0; created<num_wthreads; created++){
		retcode = pthread_create(&threads[created],NULL,worker,(void*)created);
		if(retcode){
			if(debug==1)
				printf("Error: return code from pthread_create() is %d\n", retcode);
			break;
		}	
	}
	if(debug==1){
		printf("%ld/%d Worker Threads Created Successfully\n", created, num_wthreads);
	}

	/* INITIALIZE EXTERNAL VARS (MUTEX LOCKS ETC...) */
	pthread_mutex_init(&wqlock, NULL);					// initialize work queue lock
	pthread_mutex_init(&q1lock, NULL);					// initialize q1 lock
	pthread_mutex_init(&q2lock, NULL);					// initialize q2 lock
	pthread_mutex_init(&q3lock, NULL);					// initialize q3 lock
	pthread_mutex_init(&seqlock, NULL);					// initialize sequence lock
	pthread_mutex_init(&cntlock, NULL);					// initialize count lock
	
	/* ADMIT ANY WAITING CONNECTIONS INTO WORK QUEUE TO BE PROCESSED */
	while(1){
		network_wait();
		while(cnt>=100){
				// block while there are 100 or more requests being serviced
			}		
		while((client_socket = network_open()) != -1){
			/* if network_open() returns a positive integer, push the socket to the 
			work queue */
			if(loopcount==0)									// first time through loop
				pthread_mutex_lock(&wqlock);				// lock work queue
			wqpush(client_socket);							// push job to work queue
			pthread_mutex_lock(&cntlock);					// lock cnt lock
			cnt++;												// increment cnt
			pthread_mutex_unlock(&cntlock);				// unlock cnt lock
			loopcount++;										// increment turn
		}
		loopcount=0;											// set turn back to 0
		pthread_mutex_unlock(&wqlock);					// unlock work queue
	}
	return 0;		
}

/******************************************************************************
 *                           EXTERNAL FUNCTIONS                               *
 * ****************************************************************************/

/*
 * For debugging purposes or if no port is specified on the command line. 
 * Simply requests a port number and returns that value
 */
int request_port(void) {
	int requested_port = 0;
	printf("Enter a port number between 1024 and 65535: ");
	/* Scan user input for port and store for later use */
	scanf("%d", &requested_port);
	return requested_port;
}

int ask_threads(void){
	int num_threads = 0;
	printf("Enter how many worker threads you would like to create: ");
	/* Scan user input for number of threads and store for later use */
	scanf("%d", &num_threads);
	return num_threads;
}

/*
 *  For debugging purposes - validates that specified port is acceptable. If 
 *  not, continues requesting a new port until an acceptable value has been 
 *  entered at which point this value returned to the main and stored as a 
 *  variable to be used when initializing the network 
 */
int validate_port(int validate_port) {
	while((validate_port<1024) | (validate_port>65535)) {
		printf("\nInvalid Port: %d...\n", validate_port);
		validate_port = request_port();
	}
	printf("Port Validated\n");
	printf("Waiting for Client Connection\n");
	return validate_port;
}

/*
 * Scheduler not declared on command line. Request scheduler from user.
 */
int request_scheduler(void) {
	int scheduler = -1;
	int desired_scheduler = -1;
	while(scheduler < 0) {
		printf("\n**************************************************");
		printf("**************************************************\n");
		printf("The Following Schedulers are supported in this program:\n\n");
		printf("Shortest Job First (SJF) - 0\n");
		printf("Round Robbin (RR) - 1\n");
		printf("Multi Level Queue with Feedback (MLFB) - 2\n\n");
		printf("Please Select Which Scheduler You Would Like To Use: ");
		scanf("%d", &desired_scheduler);
		printf("**************************************************");
		printf("**************************************************\n");
		if(desired_scheduler==0){
			scheduler = 0;
		} else if(desired_scheduler==1){
			scheduler = 1;
		} else if(desired_scheduler==2){
			scheduler = 2;
		}
	}
	return scheduler;
}

/* 
 * Scheduler declared on command line. Interpret command line arguement and 
 * return integer value corresponding to one of the three implemented scheduler. 
 * Request scheduler if unknown scheduler entered on command line
 */
int determine_scheduler(char *desired_scheduler){
	int scheduler = -1;
	if(strcmp(desired_scheduler,"SJF")==0){
		scheduler = 0;
	} else if(strcmp(desired_scheduler,"RR")==0){
		scheduler = 1;
	} else if(strcmp(desired_scheduler,"MLFB")==0){
		scheduler = 2;
	}
	if(scheduler<0){
		scheduler = request_scheduler();
	}
	return scheduler;
}

int request_cache(void) {
	int requested_size = 0;
	printf("Enter the cache size you would like to use: ");
	/* Scan user input for port and store for later use */
	scanf("%d", &requested_size);
	return requested_size;
}


/******************************************************************************
 *                              END OF FILE                                   *
 * ****************************************************************************/
		

