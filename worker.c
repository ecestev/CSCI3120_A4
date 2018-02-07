#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "network.h"
#include "extern.h"
#include "universal_functions.h"

void *worker(void *threadid){
	long id = (long)threadid;		// For debugging purposes
	int client_socket = -1;			// socket the client connected on
	/* string to be parsed */
	char *to_parse = calloc(256, sizeof(char));		
	/* array where the request to be parsed is stored */
	char **request_elements = calloc(3, sizeof(char**));	
	char *location;					// location of requested file
	FILE *requested_file;			// the requested file
	struct workqueue *job;			// job to be scheduled
	struct q1node *q1processing;	// job from q1
	struct q2node *q2processing;	// job from q2
	struct q3node *q3processing;	// job from q3
	char *fileip;						// filename of job in progress
	int seqip;							// sequence number of job in progress
	int sockip;							// socket number of job in progress
	int fdip;							// file descriptor of job in progress
	float blip;							// bytes left of job in progress
	int quantip;						// quantum for job in progress

	if(debug==1)
		printf("Thread %ld Created\n", id);
	
	while(1){
		pthread_mutex_lock(&wqlock);						// lock work queue
		if(!wqempty()){										// is work queue empty
			job=wqpop();										// pop job from work queue
			pthread_mutex_unlock(&wqlock);				// unlock work queue
			client_socket = job->socket;					// determine client socket	
			read(client_socket, to_parse, 256);			// read request into buffer
			parse(request_elements, to_parse);			// parse request from buffer
			/* if any of the three required HTTP request elements are missing,
			 * send error 400 malformed request on client socket */
			if((request_elements[0]==NULL)|(request_elements[1]==NULL)|
			(request_elements[2]==NULL)){	
				msg400(client_socket);						// bad request. Error 400
				close(client_socket);						// close socket connection
				pthread_mutex_lock(&cntlock);				// lock count lock
				cnt--;											// decrement count
				pthread_mutex_unlock(&cntlock);			// unlock count lock
			} else {
				/* if the request is valid, look for the requested file */
				if((strcmp(request_elements[0],"GET")==0)&&
				(strcmp(request_elements[2],"HTTP/1.1")==0)){
					location = request_elements[1];
					location++;
					/* if requested file is where it should be, open it and get 
					 * required information on the file then add the job to the 
					 * queue */
					pthread_mutex_lock(&seqlock);			// lock sequence lock
					if(fopen( location, "r" ) != NULL){
						requested_file = fopen( location, "r");
						int fd = -1;							// store file descriptor
						fd = fileno(requested_file);		// store file descriptor
						struct stat buf;						// create a buffer
						fstat(fd, &buf);						// retrieve file statistics
						float filesize = buf.st_size;		// find file size
						sequence++;								// increment seq counter
						pthread_mutex_unlock(&seqlock);	// unlock sequence lock
						/* Request confirmed valid. Push request Queue 1 and print
						 * to stdout that the job was admitted. If scheduler mode
						 * is SJF, sort the queue */
						pthread_mutex_lock(&q1lock);		// lock queue1
						push_q1(&location, sequence, client_socket, fd, 
						filesize, quant1);
						printf("Request for file %s admitted.\n", location);
						if (mode==0)							// if SJF scheduler mode
							q1sort();							// sort queue by job size												
						pthread_mutex_unlock(&q1lock);	// unlock queue1
						msg200(client_socket);				// request ok!
					} else {
						pthread_mutex_unlock(&seqlock);
			 			msg404(client_socket);				// fopen() = NULL. Error 400
						close(client_socket);				// close client connection
						pthread_mutex_lock(&cntlock);		// lock count lock
						cnt--;									// decrement count
						pthread_mutex_unlock(&cntlock);	// unlock count lock
					}
				} 	else {
					msg400(client_socket);					// bad request. Error 400
					close(client_socket);					// close client connection
					pthread_mutex_lock(&cntlock);			// lock count lock
					cnt--;										// decrement count
					pthread_mutex_unlock(&cntlock);		// unlock count lock
				}
			}
			/* ensure request array is cleaned up */
			int i=0;
			for(i=0;i<3;i++){
				if(request_elements[i]!=NULL)
					memset(request_elements[i], '\0', 
					strlen(request_elements[i]));
			}			

		/* This section is entered if the work queue ie empty only. At this 
		 * point the program will begin checking queues in order of priority
		 * and processing requests if appropriate */
		} else {													// work queue is empty
			pthread_mutex_lock(&q1lock);					// lock q1
			if(!q1_Empty()){									// check if q1 has entries;
				pthread_mutex_unlock(&wqlock);			// unlock work queue
				q1processing = pop_q1();					// pop job from queue 1
				pthread_mutex_unlock(&q1lock);			// unlock q1
				fileip = q1processing->location;			// location of popped job
				seqip = q1processing->sequence;			// sequence of popped job
				sockip = q1processing->socket;			//	socket of popped job
				fdip = q1processing->fd;					// fd of popped job
				blip = q1processing->bytes_left;			//	bytes left of popped job
				quantip = q1processing->quantum;			//	quantum of popped job			
				if(quantip!=0){								// ensure popped job is valid
					/* if there are less bytes left to send than the allowable 
					 * quantum for the specified scheduler then send the rest
					 * of the file. */
					if(blip<=quantip){						// logic check to enter if
						/* seek place in file and send the rest */
						lseek(fdip, (off_t)-blip, SEEK_END);
						sendfile(sockip, fdip, NULL, blip);	
						pthread_mutex_lock(&cntlock);		// lock count lock
						cnt--;									// decrement job count
						pthread_mutex_unlock(&cntlock);	// unlock count lock
						if(debug==1)
								printf("Sent %.0f bytes of %s in Queue 1\n", 
								blip, fileip);
						else
							printf("Sent %.0f bytes of %s\n", 
							blip, fileip);																
						fflush(stdout);						// flush stdout buffer
						printf("Request for file %s completed\n", fileip);
						fflush(stdout);						// flush stdout buffer
						close(fdip);							// close fd in progress
						close(sockip);							// close socket in progress
					/* the remaining amount of data to send exceeds the quantum.
					 * Send as much of the file as is permitted and re-queue 
					 * the remainder of the file */
					} else {
						sendfile(sockip,fdip,NULL,quantip);
						if(debug==1)
							printf("Sent %d bytes of %s in Queue 1\n", 
							quantip, fileip);
						else
							printf("Sent %d bytes of %s\n", 
							quantip, fileip);
						fflush(stdout);
						blip=blip-quantip;					// update bytes left to send
						/* if scheduler mode is MLFB push remainder of job to the 
						 * next priority queue */
						if(mode==2){
							pthread_mutex_lock(&q2lock);
							push_q2(&fileip, seqip, sockip, fdip, blip, quant2);
							pthread_mutex_unlock(&q2lock);
						/* otherwise simply re-queue */
						} else {
							pthread_mutex_lock(&q1lock);
							push_q1(&fileip, seqip, sockip, fdip, blip, quant1);
							pthread_mutex_unlock(&q1lock);
						}
					}
				/* job popped from queue was not valid. Requeue and break */
				} else {
					pthread_mutex_lock(&q1lock);
					push_q1(&fileip, seqip, sockip, fdip, blip, quant1);
					pthread_mutex_unlock(&q1lock);
				}
			/* work queue and queue 1 are both empty... See if there are any
			 * jobs in queue 2 and process if so */
			} else {
				pthread_mutex_lock(&q2lock);				// lock q2
				if(!q2_Empty()){								// does q2 have entries
					pthread_mutex_unlock(&wqlock);		// unlock work queue
					pthread_mutex_unlock(&q1lock);		// unlock q1
					q2processing = pop_q2();				// pop job from q2
					pthread_mutex_unlock(&q2lock);		// unlock q2
					fileip = q2processing->location;
					seqip = q2processing->sequence;		// sequence of popped job
					sockip = q2processing->socket;		//	socket of popped job
					fdip = q2processing->fd;				// fd of popped job
					blip = q2processing->bytes_left;		//	bytes left of popped job
					quantip = q2processing->quantum;		//	quantum of popped job			
					if(quantip!=0){							// ensure popped job is valid
						/* if there are less bytes left to send than the allowable 
						 * quantum for the specified scheduler then send the rest
						 * of the file. */
						if(blip<=quantip){				
							/* seek place in flie and send the rest */
							lseek(fdip, (off_t)-blip, SEEK_END);
							sendfile(sockip, fdip, NULL, blip);	
							pthread_mutex_lock(&cntlock);
							cnt--;
							pthread_mutex_unlock(&cntlock);
							if(debug==1)
								printf("Sent %.0f bytes of %s in Queue 2\n", 
								blip, fileip);
							else
								printf("Sent %.0f bytes of %s\n", 
								blip, fileip);
							fflush(stdout);					// flush stdout buffer
							printf("Request for file %s completed\n", fileip);
							fflush(stdout);					// flush stdout buffer
							close(fdip);						// close fd in progress
							close(sockip);						// close socket in progress
						/* the remaining amount of data to send exceeds the quantum.
						 * Send as much of the file as is permitted and re-queue 
						 * the remainder of the file */
						} else {
							sendfile(sockip,fdip,NULL,quantip);
							if(debug==1)
								printf("Sent %d bytes of %s in Queue 2\n", 
								quantip, fileip);
							else					
								printf("Sent %d bytes of %s\n", 
								quantip, fileip);
							fflush(stdout);
							blip=blip-quantip;				// update bytes left to send
							/* lock next priotiy queue and push remainder of job.
							 * unlock queue when operation complete */
							pthread_mutex_lock(&q3lock);
							push_q3(&fileip, seqip, sockip, fdip, blip, quant3);
							pthread_mutex_unlock(&q3lock);
						}
					/* job popped from queue was not valid. Requeue and break */
					} else {
						pthread_mutex_lock(&q2lock);
						push_q2(&fileip, seqip, sockip, fdip, blip, quant2);
						pthread_mutex_unlock(&q2lock);
					}
				/* work queue, queue 1, and queue 2 are all empty... See if there
				 *  are any jobs in queue 3 and process if so */
				} else {
					pthread_mutex_lock(&q3lock);			//	lock queue 3
					if(!q3_Empty()){							// does queue 3 have entries
						pthread_mutex_unlock(&wqlock);	// unlock work queue
						pthread_mutex_unlock(&q1lock);	// unlock queue 1
						pthread_mutex_unlock(&q2lock);	// unlock queue 2
						q3processing = pop_q3();			// pop job from queue 3
						pthread_mutex_unlock(&q3lock);	// unlock queue 3
						fileip = q3processing->location;
						seqip = q3processing->sequence;	// sequence of popped job
						sockip = q3processing->socket;	//	socket of popped job
						fdip = q3processing->fd;			// fd of popped job
						blip = q3processing->bytes_left;	//	bytes left of popped job
						quantip = q3processing->quantum;	//	quantum of popped job			
						if(quantip!=0){						// ensure popped job is valid
							/* if there are less bytes left to send than the allowable 
							 * quantum for the specified scheduler then send the rest
							 * of the file. */
							if(blip<=quantip){		
								lseek(fdip, (off_t)-blip, SEEK_END);// set loc in file
								sendfile(sockip, fdip, NULL, blip);	// send entire file
								pthread_mutex_lock(&cntlock);
								cnt--;
								pthread_mutex_unlock(&cntlock);
								if(debug==1)
									printf("Sent %.0f bytes of %s in Queue 3\n", 
									blip, fileip);
								else
									printf("Sent %.0f bytes of %s\n", 
									blip, fileip);
								fflush(stdout);				// flush stdout buffer
								printf("Request for file %s completed\n", fileip);
								fflush(stdout);				// flush stdout buffer
								close(fdip);					// close fd in progress
								close(sockip);					// close socket in progress
							/* the remaining amount of data to send exceeds the quantum.
							 * Send as much of the file as is permitted and re-queue 
							 * the remainder of the file */
							} else {
								sendfile(sockip,fdip,NULL,quantip);
								if(debug==1)
									printf("Sent %d bytes of %s in Queue 3\n", 
									quantip, fileip);
								else
									printf("Sent %d bytes of %s\n", 
									quantip, fileip);
								fflush(stdout);
								blip=blip-quantip;			// update bytes left to send
								/* lock queue and push remainder of job. unlock queue 
								 * when operation complete */
								pthread_mutex_lock(&q3lock);
								push_q3(&fileip, seqip, sockip, fdip, blip, quant3);
								pthread_mutex_unlock(&q3lock);
							}
						/* job popped from queue was not valid. Requeue and break */
						} else {
							pthread_mutex_lock(&q3lock);
							push_q3(&fileip, seqip, sockip, fdip, blip, quant3);
							pthread_mutex_unlock(&q3lock);
						}
					/* nothing in any queues. releas all locks and wait until there
					 * is more work to be done */
					} else {
						pthread_mutex_unlock(&wqlock);	// unlock work queue
						pthread_mutex_unlock(&q1lock);	// unlock queue 2
						pthread_mutex_unlock(&q2lock);	// unlock queue 2
						pthread_mutex_unlock(&q3lock);	// unlock queue 3
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}

