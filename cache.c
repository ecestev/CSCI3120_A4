#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "universal_functions.h"
#include "extern.h"
#include "network.h"

/* this function allocates a cache of specified size */
void cache_init(int size) {
	// malloc a cache
}

/* if the file is not in the cache, load it into the cache. Return
 * a 'cached file descriptor' (CFD) which refers to the file in the
 * cache. Note: Each worker thread should get its own CFD when opening 
 * a file. This function returns -1 if the file cannot be accessed. If
 * a thread requests a file that is being loaded into the cache, the 
 * thread should block until the file is loaded.
 * 
 * If the file cannot be loaded because it is too big, a CFD should still 
 * be returned.  The file should be opened and the cache should read the 
 * file from disk when it is needed instead of storing in memory.
 *
 * After a file is loaded, output File of Size <size> Cached.
 *
 * After a file is evicted, output File of size <size> Evicted.
 */
int cache_open(char *file){
	// is file in cache?
	// if not
	FILE *requested_file;
	requested_file = fopen(file, "r");
	int cfd = -1;
	int fd = -1;
	fd = fileno(requested_file);
	struct stat buf;
	fstat(fd, &buf);
	float filesize = buf.st_size;
	/* next 2 lines are literally just to eliminate a warning */
	filesize++;
	filesize--;
	// is there room in the cache?
		// if yes:
			// cache.push();
			// output to stdout
			// return cfd
		// else
			//	while(noroomincache){
				// can we evict any entries?
					// evict entry
					// output to stdout
			// }
			// is there room now?
				// yes = cache.push()
					// output to stdout
					// return cfd
			// no = return cfd
	return cfd;
}

/* the next n bytes of the file referred to by the cfd should be sent 
 * to the client by performing a write with the (client) file descriptor.
 * The value n is the maximum number of bytes to send. If the number of bytes
 * remaining is r < n then r bytes should be sent. The function returns the
 * number of bytes sent.
 *
 * If the file referred to by cfd is too big to fit into the cache, it should
 * be read from disk instead of from the cache.
 */
int cache_send(int cfd, int client, int n){
	// is there more than n bytes left to send?
		sendfile(client,cfd,NULL,n);
		return n;
	// else
		//	sendfile(client,cfd,NULL,r);
		//	return r;
}

/* returns the size of the file */
int cache_filesize(int cfd){
	struct stat buf;
	fstat(cfd, &buf);
	float filesize = buf.st_size;
	return filesize;
}

/* closes the cached file descriptor. If the file referred to by the cfd
 * is to big to fit into the cache, it should be closed */
int cache_close(int cfd){
	if(close(cfd)==0)
		return 0;
	else
		return 1;
}

