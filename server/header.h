#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include "MD5.h"
#include <sstream>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>       	 /* sockets */
#include <sys/types.h>      	 /* sockets */
#include <sys/socket.h>     	 /* sockets */
#include <netinet/in.h>     	 /* internet sockets */
#include <netdb.h>               /* gethostbyaddr */
#include <unistd.h>		/* fork */
#include <signal.h>          	/* signal */
#include <errno.h>
using namespace std;

#include "list_server.h"

#define PAGESIZE 4096
#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

struct Encoded{
        unsigned int digest[4];
};

struct internal_socket_message{
	int expect;
	int client_id;
	char filename[30];
	int size;
	struct Encoded* tree_pointer;
	int page_number;
	char page_data[PAGESIZE];
};
struct DATA_message{
        char filename[30];
        int pagenum;
//        char page_data[PAGESIZE];
        int die;
};

struct Encoded** TreeCreation(int height, int fan_out,char* filename);
struct Encoded** TreeAllocation(int height, int fan_out);
int power(int a, int b);
void Transform2D_to_1D(int height,int fan_out,struct Encoded**& level,struct Encoded*& new_level);
void Assign1D_to_2D(int height, int fan_out, struct Encoded**& level, struct Encoded*& new_level);
void* working_thread(void*);
void perror_exit(char *message);
void inserting_client_message_working_thread_list(string ,ListS*&,int);
