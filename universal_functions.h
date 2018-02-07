void parse(char **request_element, char *to_parse);
void msg400(int socket);
void msg404(int socket);
void msg200(int socket);
void filsizerr(int socket);
void set_quantums(int mode);

struct arg_struct{
	int arg1;
	int arg2;
};

struct workqueue{
	int socket;
	struct workqueue *wqnext;
};
bool wqempty(void);
void wqpush(int socket);
struct workqueue *wqpop(void);
void printwq(void);

struct q1node{
	char *location;
	int sequence;					// sequence number of the job coming in
	int socket;						// socket number to send the file on
	int fd;							// file descriptor of the file requested
	float bytes_left;				// remaining bytes left to transfer to client
	int quantum;					// how many bytes will be sent at a time
	struct q1node *q1next;		// pointer to the next node in linked list
};
bool q1_Empty(void);
int in_q1(void);
void print_q1(void);
void push_q1(char **location, int sequence, int socket, int fd, 
float bytes_left, int quantum);
struct q1node *pop_q1(void);
void q1sort(void);

struct q2node{
	char *location;
	int sequence;					// sequence number of the job coming in
	int socket;						// socket number to send the file on
	int fd;							// file descriptor of the file requested
	float bytes_left;				// remaining bytes left to transfer to client
	int quantum;					// how many bytes will be sent at a time
	struct q2node *q2next;		// pointer to the next node in linked list
};
bool q2_Empty(void);
int in_q2(void);
void print_q2(void);
void push_q2(char **location, int sequence, int socket, int fd, 
float bytes_left, int quantum);
struct q2node *pop_q2(void);

struct q3node{
	char *location;
	int sequence;					// sequence number of the job coming in
	int socket;						// socket number to send the file on
	int fd;							// file descriptor of the file requested
	float bytes_left;				// remaining bytes left to transfer to client
	int quantum;					// how many bytes will be sent at a time
	struct q3node *q3next;		// pointer to the next node in linked list
};
bool q3_Empty(void);
int in_q3(void);
void print_q3(void);
void push_q3(char **location, int sequence, int socket, int fd, 
float bytes_left, int quantum);
struct q3node *pop_q3(void);


