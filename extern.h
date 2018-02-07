#ifndef EXTERN_H
#define EXTERN_H
pthread_mutex_t wqlock;
pthread_mutex_t q1lock;
pthread_mutex_t q2lock;
pthread_mutex_t q3lock;
pthread_mutex_t seqlock;
pthread_mutex_t cntlock;
int mode;
int quant1;
int quant2;
int quant3;
int sequence;
int cnt;
int debug;
#endif

