#include <sys/time.h>
#include <time.h>

typedef struct {
	int activ ;
	char ip[20] ;
	int port ;
	char nume[200] ; 
	time_t timp ; 
	int socket ;
} client ;

typedef struct {
	int type;
	int len;
	char payload[1024];
	char sursa[1024];
	char destinatie[1024];
	client client;
	time_t timp ; 
	char nume_fisier[100];
} msg;