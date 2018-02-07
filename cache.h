void cache_init(int size);
int cache_open(char *file);
int cache_sent(int cfd, int client, int n);
int cache_filesize(int cfd);
int cache_close(int cfd);
