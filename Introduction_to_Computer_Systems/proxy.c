/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *    Tong Tzeho (1100012773), guest475@andrew.cmu.edu 
 *    Wen Shiwei (1100012968), guest407@andrew.cmu.edu
 * 
 * A proxy which works as a server to the client and as a client to
 * the real server. It can deal with multiple concurrent requests by
 * using the Pthreads lib, and it has a cache which works when con-
 * necting the same uri frequently. Additionally, it can handle POST 
 * requests.
 *
 * By testing, it can connect to the web sites inside PKU, such as:
 *
 * http://mail.pku.edu.cn
 * http://www.gotopku.cn
 * http://poj.grids.cn
 * http://eecs.bdwm.net
 * http://162.105.31.220
 *
 * It can also connect to the web sites outside PKU though it is a
 * little slow, such as:
 *
 * http://tieba.baidu.com
 * http://www.renren.com
 * http://weibo.com
 * http://www.sodasoccer.com
 * http://www.qq.com
 *
 */ 

#include "csapp.h"

#define MIN_OBJECT_SIZE 32
#define MAX_LIST0_OBJECT_SIZE 64
#define MAX_LIST1_OBJECT_SIZE 128
#define MAX_LIST2_OBJECT_SIZE 256
#define MAX_LIST3_OBJECT_SIZE 512
#define MAX_LIST4_OBJECT_SIZE 2048
#define MAX_OBJECT_SIZE 102400 /* 100 KiB */
#define MAX_OBJECT_NUM_PER_LIST 150
#define MAX_CACHE_SIZE_PER_LIST 597376 /* 1 MiB - (64+128+256+512+2048)Bytes * 150 = 597376 Bytes*/
 
/*
 * Struct for cache memory using a linked list struct
 */
struct cache_data
{
	char *uri, *data;
	struct cache_data *prev, *next;
	int content_length;
};

static struct cache_data cache_tmp, *head[6], *tail[6];

static sem_t mutex;

/* object_num[i] <= MAX_OBJECT_NUM_PER_LIST AND cache_list_size[i] <= MAX_CACHE_SIZE_PER_LIST */
static int object_num[6], cache_list_size[6], max_object_size = 0, cache_size_total = 0;

/*
 * Function prototypes
 */
void *thread(void *vargp);
void doit(int fd);
int read_cache(int sfd, char *uri, int content_length);
void write_cache(char *uri, char *buf, int content_length);
void send_request(int fd, char *method, char *path, char *version, char *hdrs);
void read_requesthdrs(rio_t *rio, char *buf, int t);
int parse_uri(char *uri, char *target_addr, char *path, int *port);
int get_content_length(char *buf);
int get_list_index(int content_length);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
	int listenfd, *connfdp, port, clientlen, i;
	struct sockaddr_in clientaddr;
	pthread_t tid;

	/* Check arguments */
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		exit(0);
	}

	/* Initialization */
	Sem_init(&mutex, 0, 1);
	for (i = 0; i < 6; i++)
	{
		head[i] = tail[i] = NULL;
		object_num[i] = cache_list_size[i] = 0;
	}
	port = atoi(argv[1]);

	/* Ignore SIGPIPE otherwise it will be terminated */
	signal(SIGPIPE, SIG_IGN);

	/* Open a listen port to accept a client request */
	listenfd = open_listenfd(port);

	/* Accept all requests */
	while (listenfd >= 0)
	{
		clientlen = sizeof(clientaddr);
		connfdp = Malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Pthread_create(&tid,NULL,thread,connfdp);
	}

	/* Control will never reach here */
	exit(0);
}

/*
 * thread - Do the concurrent requests
 */
void *thread(void *vargp)
{
	int connfd = *((int *)vargp);
	Pthread_detach(pthread_self());
	Free(vargp);
	doit(connfd);
	close(connfd);
}

/*
 * doit - Do a single request
 */
void doit(int sfd)
{
	rio_t srio, crio;
	char sbuf[MAXLINE], cbuf[MAXLINE];
	char method[MAXLINE], uri[MAXLINE], version[MAXLINE], hdrs[MAXLINE];
	char address[MAXLINE], path[MAXLINE];
	int cfd, port = 80, n, content_length, tmp, use_cache = -1;

	/* Read request line */
	rio_readinitb(&srio, sfd);
	if (rio_readlineb(&srio, sbuf, MAXLINE) < 0)
	{
		clienterror(sfd, NULL, "404", "NOT Found","The page is not found!");
		return;
	}
	sscanf(sbuf, "%s %s %s", method, uri, version);

	/* handle GET and POST requests */
	if ((strcmp(method, "GET")) && (strcmp(method, "POST")))
	{
		clienterror(sfd, method, "501", "Not Implemented", "Tiny does not implement this method");
		return;
	}

	/* Read request headers and request body (POST requests) */
	read_requesthdrs(&srio, hdrs, strcmp(method, "POST"));

	/* Parse URI form GET or POST requests */
	parse_uri(uri, address, path, &port);

	if ((cfd = open_clientfd(address, port)) < 0)
	{
		return;
	}

	/* Send requests to the server as a client */
	send_request(cfd, method, path, version, hdrs);

	rio_readinitb(&crio, cfd);

	printf("Connected to server : %s\n", uri);
	fflush(stdout);

	/* Read the header ending with "\r\n" from the server as a client */
	do
	{
		n = rio_readlineb(&crio, cbuf, MAXLINE);
		if (!strncasecmp(cbuf, "Content-length: ", 16))
		{
			tmp = content_length = get_content_length(cbuf);
		}
		rio_writen(sfd, cbuf, n);
	}
	while(strcmp(cbuf, "\r\n"));

	/* Search the same URI in cache memory */
	if ((content_length != 0) && (content_length <= MAXBUF))
	{
		use_cache = read_cache(sfd, uri, content_length);

		/* Read cache successfully */
		if (use_cache > 0)
		{
			/* Old version need update */
			if (use_cache == 1)
			{
				n = rio_readnb(&crio, cbuf, tmp);
				if ((n <= MAX_OBJECT_SIZE) && (n >= MIN_OBJECT_SIZE))
				{
					write_cache(uri, cbuf, n);
				}
			}

			close(cfd);
			return;
		}
	}

	/* Transfer the data between the server and the client */
	if (content_length == 0)
	{
		while ((n = rio_readnb(&crio, cbuf, MAXBUF)) > 0)
		{
			use_cache = 0;
			rio_writen(sfd, cbuf, n);
			content_length += n;
		}
		close(cfd);
	}
	else
	{
		while ((tmp > MAXBUF) && ((n = rio_readnb(&crio, cbuf, MAXBUF)) > 0))
		{
			use_cache = 0;
			rio_writen(sfd, cbuf, n);
			tmp -= n;
		}
		n = rio_readnb(&crio, cbuf, tmp);
		rio_writen(sfd, cbuf, n);
		close(cfd);

		/* write the data into the cache */
		if (use_cache && (n <= MAX_OBJECT_SIZE) && (n >= MIN_OBJECT_SIZE))
		{
			write_cache(uri, cbuf, n);
		}
	}
}

/*
 * read_cache - Read cache memory
 *
 * Get the data from cache memory if there is the same URI.
 * Return -1 otherwise. 
 */
int read_cache(int sfd, char *uri, int content_length)
{
	struct cache_data *cache;
	int count_size = 0, cache_id = 1, index = get_list_index(content_length);

	if (tail[index] == NULL) return -1; /* Empty cache */

	cache = tail[index];
	do
	{
		if ((cache != NULL) && (cache->uri != NULL) && (!strcasecmp(cache->uri, uri)))
		{
			rio_writen(sfd, cache->data, cache->content_length);
			return object_num[index]/cache_id;
		}

		count_size += cache->content_length;
		cache_id++;
		if ((cache_id > MAX_OBJECT_NUM_PER_LIST-5) || (count_size > MAX_CACHE_SIZE_PER_LIST-max_object_size)) break; /* Near head */
		cache = cache->prev;
		cache_id++;
	}
	while ((cache != NULL) && (cache_id < object_num[index]));
	return -1;
}

/*
 * write_cache - Write cache memory
 */
void write_cache(char *uri, char *buf, int content_length)
{
	struct cache_data *cache;
	int uri_len = strlen(uri), i, index = get_list_index(content_length);

	/* Copy the information */
	cache = Malloc(sizeof(cache_tmp));
	cache->uri = (char*)Malloc(uri_len+1);
	strncpy(cache->uri, uri, uri_len);
	cache->uri[uri_len] = '\0';
	cache->data = (char*)Malloc(content_length);
	for (i = 0; i < content_length; i++)
		cache->data[i] = buf[i];
	cache->content_length = content_length;
	if (content_length > max_object_size) max_object_size = content_length;

	/* Put the new node at the tail of the linked list */
	cache->next = NULL;
	P(&mutex);
	if (head[index] == NULL)
	{
		head[index] = tail[index] = cache;
		cache -> prev = NULL;
	}
	else
	{
		tail[index] -> next = cache;
		cache -> prev = tail[index];
		tail[index] = cache;
 	}
	object_num[index]++;
	cache_size_total += content_length;
	cache_list_size[index] += content_length;

	/* Delete node(s) at the head of the linked list if there are 
	   too many objects or the size is too large */
	while((cache_list_size[index] > MAX_CACHE_SIZE_PER_LIST) || (object_num[index] > MAX_OBJECT_NUM_PER_LIST))
	{
		cache = head[index];
		head[index] = head[index]->next;
		head[index]->prev = NULL;
		free(cache->data);
		free(cache->uri);
		cache_list_size[index] -= cache->content_length;
		cache_size_total -= cache->content_length;
		free(cache);
		object_num[index]--;
	} 
	V(&mutex);
}

/*
 * send_request - Send requests to the server as a client
 */
void send_request(int fd, char *method, char *path, char *version, char *hdrs)
{
	char buf[MAXLINE];

	sprintf(buf, "%s %s %s", method, path, version);
	sprintf(buf, "%s\r\n%s\0", buf, hdrs);

	rio_writen(fd, buf, strlen(buf));
}

/*
 * read_requesthdrs - Read and parse HTTP request headers
 */
void read_requesthdrs(rio_t *rio, char *buf, int t)
{
	char tmp[MAXLINE];
	int content_length = 0;
	*buf='\0';

	do	
	{
		rio_readlineb(rio, tmp, MAXLINE);

		/* Get the Content-length from POST requests */
		if (!t && !strncasecmp(tmp, "Content-length: ", 16))
		{
			content_length = get_content_length(tmp);
		}
		sprintf(buf, "%s%s", buf, tmp);	
	}
	while(strcmp(tmp,"\r\n"));

	/* Read request body form POST requests */
	if (!t)
	{
		rio_readnb(rio, tmp, content_length);
		sprintf(buf, "%s%s", buf, tmp);
	}
}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port. The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
	char *hostbegin;
	char *hostend;
	char *pathbegin;
	int len;

	if (strncasecmp(uri, "http://", 7) != 0)
	{
		hostname[0] = '\0';
		return -1;
	}

	/* Extract the host name */
	hostbegin = uri + 7;
	hostend = strpbrk(hostbegin, " :/\r\n\0");
	if (hostend == NULL)
	{
		for (hostend = hostbegin; ;hostend++)
			if (*hostend == '\0')break;
	}
	len = hostend - hostbegin;
	strncpy(hostname, hostbegin, len);
	hostname[len] = '\0';

	/* Extract the port number */
	*port = 80; /* default */
	if (*hostend == ':')
	{
		*port = atoi(hostend + 1);
	}

	/* Extract the path */
	pathbegin = strchr(hostbegin, '/');
	if (pathbegin == NULL)
	{
		strcpy(pathname, "/\0");
	}
	else
	{	
		strcpy(pathname, pathbegin);
	}

	return 0;
}

/*
 * get_content_length - Get content length from "Content-length : " statement
 */
int get_content_length(char *buf)
{
	char length[16], *length_begin, *length_end;
	length_begin = buf + 16;
	length_end = strpbrk(length_begin, "\r\n");
	strncpy(length, length_begin, length_end - length_begin);
	length[length_end - length_begin] = '\0';
	return atoi(length);
}

/*
 * get_list_index - Give the content_length and get the index of list
 */
int get_list_index(int content_length)
{
	if (content_length <= MAX_LIST0_OBJECT_SIZE) return 0;
	else if (content_length <= MAX_LIST1_OBJECT_SIZE) return 1;
	else if (content_length <= MAX_LIST2_OBJECT_SIZE) return 2;
	else if (content_length <= MAX_LIST3_OBJECT_SIZE) return 3;
	else if (content_length <= MAX_LIST4_OBJECT_SIZE) return 4;
	else return 5;
}

/*
 * clienterror - Returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	rio_writen(fd, buf, strlen(buf));
	rio_writen(fd, body, strlen(body));
}

