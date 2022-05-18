#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

int create_tcp_socket();
char *get_ip(char *host);
char *build_get_query(char *host, char *page);
void usage();
void *client(void *arg);
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1);

#define PAGE "/index.html"
#define USERAGENT "HTMLGET 1.0"

#define MAX_THREAD 100

char *host;
char *page;
int port;

int main(int argc, char **argv)
{
		if(argc < 3){
				usage();
				exit(2);
		}

  struct timeval tvBegin, tvEnd, tvDiff;
		int nthread, failed = 0;
		int *tret = NULL;
				
		if(argc < 3) {
				printf("usage: ./client [server ip or dns] [port] <# thread>\n");
				return 0;
		}
		host = argv[1];
		port = atoi(argv[2]);
		
		if(argc < 4) nthread  = 10;
		else nthread = atoi(argv[3]);
		if(nthread > MAX_THREAD) nthread = MAX_THREAD;
		if(argc > 4){
				page = argv[4];
		} else{
				page = PAGE;
		}

		gettimeofday(&tvBegin, NULL);
		
		printf("Request: GET %s:%d/%s, # of client: %d\n", host, port, page, nthread);

		pthread_t p[MAX_THREAD];
		int i;
		for(i = 0; i < nthread; i++)
		{
				pthread_create(&(p[i]), NULL, client, NULL);
		}

		for(i = 0; i < nthread; i++)
		{
				pthread_join(p[i], (void**)&tret);
				while((*tret) <= 0) {
						pthread_create(&(p[i]), NULL, client, NULL);
						pthread_join(p[i], (void**)&tret);
						failed++;
				}
		}
		gettimeofday(&tvEnd, NULL);
		timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
		printf("Time to handle %d requests (%d failed): %ld.%06ld sec\n", nthread, failed, tvDiff.tv_sec, tvDiff.tv_usec);
}

void *client(void *arg)
{
		struct sockaddr_in *remote;
		int sock;
		int tmpres;
		char *get;
		char buf[BUFSIZ+1];
		char **argv = arg;
		char *ip;

  struct timeval ltvBegin, ltvEnd, ltvDiff;

		gettimeofday(&ltvBegin, NULL);
		sock = create_tcp_socket();
		ip = get_ip(host);
		//fprintf(stderr, "IP is %s:%d\n", ip, port); 
		remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
		remote->sin_family = AF_INET;
		tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
		if( tmpres < 0)  
		{
				perror("Can't set remote->sin_addr.s_addr");
				exit(1);
		} else if(tmpres == 0)
		{
				fprintf(stderr, "%s is not a valid IP address\n", ip);
				exit(1);
		}
		remote->sin_port = htons(port);

		if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
				perror("Could not connect");
				exit(1);
		}

		get = build_get_query(host, page);
		//fprintf(stderr, "Query is:\n<<START>>\n%s<<END>>\n", get);

		//Send the query to the server
		int sent = 0;
		while(sent < strlen(get))
		{ 
				tmpres = send(sock, get+sent, strlen(get)-sent, 0);
				if(tmpres == -1){
						perror("Can't send query");
						exit(1);
				}
				sent += tmpres;
		}
		//now it is time to receive the page
		memset(buf, 0, sizeof(buf));
		int htmlstart = 0;
		char * htmlcontent;
		int rbyte = 0;
		while((tmpres = recv(sock, buf, BUFSIZ, 0)) > 0)
		//while((tmpres = read(sock, buf, BUFSIZ)) > 0)
				rbyte += tmpres;

		if(tmpres < 0)
		{
				perror("Error receiving data");
		}

		gettimeofday(&ltvEnd, NULL);
		timeval_subtract(&ltvDiff, &ltvEnd, &ltvBegin);
		if(tmpres >= 0) printf("[tid %ld] received %d bytes (%ld.%06ld sec).\n", syscall(186), rbyte, ltvDiff.tv_sec, ltvDiff.tv_usec);
		free(get);
		free(remote);
		free(ip);
		close(sock);
		pthread_exit((void*)&rbyte);
}

void usage()
{
		fprintf(stderr, "USAGE: ./client <host> <port> <# thread> [page]\n\
						\thost: IP or hostname. ex: 127.0.0.1\n\
						\tpage: the page to retrieve. ex: index.html, default: /\n");
}


int create_tcp_socket()
{
		int sock;
		if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
				perror("Can't create TCP socket");
				exit(1);
		}
		return sock;
}


char *get_ip(char *host)
{
		struct hostent *hent;
		int iplen = 15; //XXX.XXX.XXX.XXX
		char *ip = (char *)malloc(iplen+1);
		memset(ip, 0, iplen+1);
		if((hent = gethostbyname(host)) == NULL)
		{
				herror("Can't get IP");
				exit(1);
		}
		if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
		{
				perror("Can't resolve host");
				exit(1);
		}
		return ip;
}

char *build_get_query(char *host, char *page)
{
		char *query;
		char *getpage = page;
		char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
		if(getpage[0] == '/'){
				getpage = getpage + 1;
				//fprintf(stderr,"Removing leading \"/\", converting %s to %s\n", page, getpage);
		}
		// -5 is to consider the %s %s %s in tpl and the ending \0
		query = (char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tpl)-5);
		sprintf(query, tpl, getpage, host, USERAGENT);
		return query;
}

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
		long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
		result->tv_sec = diff / 1000000;
		result->tv_usec = diff % 1000000;

		return (diff<0);
}
