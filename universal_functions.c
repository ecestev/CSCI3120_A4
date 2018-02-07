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

/*
 * linked list implementation adapted from: http://tutorialspoint.com/
 * data_structures_algorithms/linked_list_program_in_c.html
 */
 
/******************************************************************************
*						WORK QUEUE STRUCTS AND FUNCTIONS
******************************************************************************/

struct workqueue{
	int socket;
	struct workqueue *wqnext;
};
struct workqueue *wqhead = NULL;
struct workqueue *wqcurrent = NULL;

bool wqempty(void){
	return wqhead==NULL;
}

void wqpush(int socket){
	int empty = 0;
	if(wqempty())
		empty = 1;
	struct workqueue *link = malloc(sizeof(struct workqueue));
	struct workqueue *ptr = wqhead;
	link->socket = socket;
	if(empty==1){
		link->wqnext = wqhead;
		wqhead = link;
	} else {
		while(ptr->wqnext!=NULL)
			ptr=ptr->wqnext;
		ptr->wqnext = link;
	}
}

struct workqueue *wqpop(void){
	struct workqueue *tempLink = wqhead;
	wqhead = wqhead->wqnext;
	return tempLink;
}

void printwq(void){
	struct workqueue *ptr = wqhead;
	printf("\n[");
	while(ptr!=NULL){
		printf("(%d)", ptr->socket);
		ptr=ptr->wqnext;
	}
	printf("]\n\n");
}

/******************************************************************************
*						Q1 STRUCTS AND FUNCTIONS
******************************************************************************/

struct q1node{
	char *location;					// location of requested file
	int sequence;						// sequence number of the job coming in
	int socket;							// socket number to send the file on
	int fd;								// file descriptor of the file requested
	float bytes_left;					// remaining bytes left to transfer to client
	int quantum;						// how many bytes will be sent at a time
	struct q1node *q1next;			// pointer to the next node in linked list
};
struct q1node *q1head = NULL;		// pointer to the head of the linked list
struct q1node *q1current = NULL;	// last entry in linked list points to NULL


bool q1_Empty(void){
	return q1head == NULL;
}

/* DETERMINE NUMBER OF ELEMENTS IN LIST */
int in_q1(void) {
	int length = 0;
	struct q1node *q1current;
	for(q1current = q1head; q1current != NULL; q1current = q1current->q1next){
		length++;
	}
	return length;
}

/* PRINT THE LINKED LIST TO CONSOLE */
void print_q1(void){
	struct q1node *ptr = q1head;
	printf("\n[");
	while(ptr != NULL){
		printf("(%s,%d,%d,%d,%f,%d)",ptr->location,ptr->sequence, ptr->socket, 
		ptr->fd, ptr->bytes_left, ptr->quantum);
		ptr = ptr->q1next;
	}
	printf("]\n\n");
}

/* ADD ENTRY TO FRONT OF LINKED LIST */
void push_q1(char **location, int sequence, int socket, int fd, 
float bytes_left, int quantum){
	int empty = 0;
	if(q1_Empty())
		empty = 1;
	struct q1node *link = malloc(sizeof(struct q1node));
	struct q1node *ptr = q1head;
	link->location = calloc(256, sizeof(char));
	strncpy(link->location, *location, strlen(*location));
	link->sequence = sequence;
	link->socket = socket;
	link->fd = fd;
	link->bytes_left = bytes_left;
	link->quantum = quantum;
	
	if(empty==1){
		link->q1next = q1head;
		q1head = link;
	} else {
		while(ptr->q1next!=NULL){
			ptr=ptr->q1next;
		}
		ptr->q1next = link;
	}
	
}

/* REMOVE ENTRY FROM FRONT OF LINKED LIST */
struct q1node *pop_q1(void){
	struct q1node *tempLink = q1head;
	q1head = q1head->q1next;
	return tempLink;
}

/* SORT LINKED LIST BY INCREASING VALUE OF 'BYTES_LEFT' */
void q1sort(void){
	int i, j, k, tempseq, tempsock, tempfd, tempquant;
	char *temploc;
	float tempbl;
	struct q1node *q1current;
	struct q1node *q1next;
	int size = in_q1();
	k = size;
	for(i=0;i<size-1;i++,k--){
		q1current = q1head;
		q1next = q1head->q1next;
		for(j=1;j<k;j++){
			/* CHECK IF NEXT ENTRY HAS MORE BYTES LEFT THAN CURRENT ENTRY.
			 * IF SO, SWAP THEM */
			if(q1current->bytes_left > q1next->bytes_left) {
				
				temploc = q1current->location;
				q1current->location = q1next->location;
				q1next->location = temploc;

				tempseq = q1current->sequence;
				q1current->sequence = q1next->sequence;
				q1next->sequence = tempseq;
				tempsock=q1current->socket;
				q1current->socket = q1next->socket;
				q1next->socket = tempsock;
				tempfd = q1current->fd;
				q1current->fd = q1next->fd;
				q1next->fd = tempfd;
				tempbl = q1current->bytes_left;
				q1current->bytes_left = q1next->bytes_left;
				q1next->bytes_left = tempbl;
				tempquant = q1current->quantum;
				q1current->quantum = q1next->quantum;
				q1next->quantum = tempquant;
			/* IF CURRENT ENTRY AND NEXT ENTRY HAVE THE SAME AMOUNT OF BYTES
			 * REMAINING TO TRANSFER, SORT THEM IN ORDER OF SEQUENCE NUMBER */
			} 	else if((q1current->bytes_left==q1next->bytes_left)&&
			(q1current->sequence>q1next->sequence)){
				
				temploc = q1current->location;
				q1current->location = q1next->location;
				q1next->location = temploc;

				tempseq = q1current->sequence;
				q1current->sequence = q1next->sequence;
				q1next->sequence = tempseq;
				tempsock=q1current->socket;
				q1current->socket = q1next->socket;
				q1next->socket = tempsock;
				tempfd = q1current->fd;
				q1current->fd = q1next->fd;
				q1next->fd = tempfd;
				tempbl = q1current->bytes_left;
				q1current->bytes_left = q1next->bytes_left;
				q1next->bytes_left = tempbl;
				tempquant = q1current->quantum;
				q1current->quantum = q1next->quantum;
				q1next->quantum = tempquant;
			}
			q1current = q1current->q1next;
			q1next = q1next->q1next;
		}
	}
}


/******************************************************************************
*						Q2 STRUCTS AND FUNCTIONS
******************************************************************************/
struct q2node{
	char *location;					// location of requested file
	int sequence;						// sequence number of the job coming in
	int socket;							// socket number to send the file on
	int fd;								// file descriptor of the file requested
	float bytes_left;					// remaining bytes left to transfer to client
	int quantum;						// how many bytes will be sent at a time
	struct q2node *q2next;			// pointer to the next node in linked list
};
struct q2node *q2head = NULL;		// pointer to the head of the linked list
struct q2node *q2current = NULL; // last entry in linked list points to NULL

bool q2_Empty(void){
	return q2head == NULL;
}

/* DETERMINE NUMBER OF ELEMENTS IN LIST */
int in_q2(void) {
	int length = 0;
	struct q2node *q2current;
	for(q2current = q2head; q2current != NULL; q2current = q2current->q2next){
		length++;
	}
	return length;
}

/* PRINT THE LINKED LIST TO CONSOLE */
void print_q2(void){
	struct q2node *ptr = q2head;
	printf("\n[");
	while(ptr != NULL){
		printf("(%s,%d,%d,%d,%f,%d)",ptr->location, ptr->sequence, ptr->socket, 
		ptr->fd, ptr->bytes_left, ptr->quantum);
		ptr = ptr->q2next;
	}
	printf("]\n\n");
}

/* ADD ENTRY TO FRONT OF LINKED LIST */
void push_q2(char **location, int sequence, int socket, int fd, 
float bytes_left, int quantum){
	int empty = 0;
	if(q2_Empty())
		empty = 1;
	struct q2node *link = malloc(sizeof(struct q2node));
	struct q2node *ptr = q2head;
	link->location = calloc(256, sizeof(char));
	strncpy(link->location, *location, strlen(*location));
	link->sequence = sequence;
	link->socket = socket;
	link->fd = fd;
	link->bytes_left = bytes_left;
	link->quantum = quantum;
	if(empty==1){
		link->q2next = q2head;
		q2head = link;
	} else {
		while(ptr->q2next!=NULL){
			ptr=ptr->q2next;
		}
		ptr->q2next = link;
	}
}

/* REMOVE ENTRY FROM FRONT OF LINKED LIST */
struct q2node *pop_q2(void){
	struct q2node *tempLink = q2head;
	q2head = q2head->q2next;
	return tempLink;
}

/******************************************************************************
*						Q3 STRUCTS AND FUNCTIONS
******************************************************************************/
struct q3node{
	char *location;						// location of requested file
	int sequence;							// sequence number of the job coming in
	int socket;								// socket number to send the file on
	int fd;									// file descriptor of the file requested
	float bytes_left;						// remaining bytes left to send to client
	int quantum;							// how many bytes will be sent at a time
	struct q3node *q3next;				// pointer to the next node in linked list
};
struct q3node *q3head = NULL;			// pointer to the head of the linked list
struct q3node *q3current = NULL; 	// last entry in linked list points to NULL

bool q3_Empty(void){
	return q3head == NULL;
}

/* DETERMINE NUMBER OF ELEMENTS IN LIST */
int in_q3(void) {
	int length = 0;
	struct q3node *q3current;
	for(q3current = q3head; q3current != NULL; q3current = q3current->q3next){
		length++;
	}
	return length;
}

/* PRINT THE LINKED LIST TO CONSOLE */
void print_q3(void){
	struct q3node *ptr = q3head;
	printf("\n[");
	while(ptr != NULL){
		printf("(%s,%d,%d,%d,%f,%d)",ptr->location, ptr->sequence, ptr->socket, 
		ptr->fd, ptr->bytes_left, ptr->quantum);
		ptr = ptr->q3next;
	}
	printf("]\n\n");
}

/* ADD ENTRY TO FRONT OF LINKED LIST */
void push_q3(char **location, int sequence, int socket, int fd, 
float bytes_left, int quantum){
	int empty = 0;
	if(q3_Empty())
		empty = 1;
	struct q3node *link = malloc(sizeof(struct q3node));
	struct q3node *ptr = q3head;
	link->location = calloc(256, sizeof(char));
	strncpy(link->location, *location, strlen(*location));
	link->sequence = sequence;
	link->socket = socket;
	link->fd = fd;
	link->bytes_left = bytes_left;
	link->quantum = quantum;
	if(empty==1){
		link->q3next = q3head;
		q3head = link;
	} else {
		while(ptr->q3next!=NULL){
			ptr=ptr->q3next;
		}
		ptr->q3next = link;
	}
}

/* REMOVE ENTRY FROM FRONT OF LINKED LIST */
struct q3node *pop_q3(void){
	struct q3node *tempLink = q3head;
	q3head = q3head->q3next;
	return tempLink;
}

/******************************************************************************
*   						OTHER UNIVERSAL FUNCTIONS
******************************************************************************/

/*
 * A simple function to parse the HTTP request. This function
 * will store as many white space separated series of characters
 * as a string until there is no more room in the array of strings.
 */

void parse(char **request_elements, char *to_parse){
	char **ap;
 	char *inputstring = to_parse;
	for(ap=request_elements;(*ap=strsep(&inputstring, " \t\n\r")) != NULL;)
		if(**ap != '\0')
			if(++ap >= &request_elements[3])
				break;
}
	
void msg400(int socket) {
	char *msg400	= "HTTP/1.1 400 BAD REQUEST\n\n"; 			  /* response 400 */
	write(socket, msg400, strlen(msg400));
}

void msg404(int socket) {
	char *msg404 = "HTTP/1.1 404 FILE NOT FOUND\n\n"; 			  /* response 404 */
	write(socket, msg404, strlen(msg404));
}

void msg200(int socket) {
	char *msg200 = "HTTP/1.1 200 OK\n\n"; 							  /* response 200 */
	write(socket, msg200, strlen(msg200));
}

void filsizerr(int socket) {
	char *filsizerr = "Requested File Exceeds capabilities of sendfile()\n\n";
	write(socket, filsizerr, strlen(filsizerr));
}

void set_quantums(int mode){
	if(mode==0)
		quant1=2147479552;
	if(mode==1)
		quant1=8192;
	if(mode==2){
		quant1=8192;
		quant2=65535;
		quant3=65535;
	}
}

