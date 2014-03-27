#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mesaj.h"

#define MAX_CLIENTS	100
#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(1);
}

client clienti[MAX_CLIENTS] ;
int nr_clienti = 0 ;
time_t timp ;

int main(int argc, char *argv[]) {
     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFLEN];
     struct sockaddr_in serv_addr, cli_addr;
     int n, i, j, k;

     int se_inchide = 0; 

     msg mesaj ;
  
     fd_set read_fds;	
     fd_set tmp_fds;	
     int fdmax;		

     if (argc < 2) {
         fprintf(stderr,"Usage : %s port\n", argv[0]);
         exit(1);
     }

     FD_ZERO(&read_fds);
     FD_ZERO(&tmp_fds);
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     portno = atoi(argv[1]);
     timp = time(NULL);

     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;	
     serv_addr.sin_port = htons(portno);

     bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
     listen(sockfd, MAX_CLIENTS);

     FD_SET(fileno(stdin), &read_fds);
     FD_SET(sockfd, &read_fds);
     fdmax = sockfd;

     while (1) {
		tmp_fds = read_fds; 
		select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) ;
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {

					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
				else 
					if ( i == 0 ) { 
			    		memset(buffer, 0 , BUFLEN);
			    		fgets(buffer, BUFLEN-1, stdin); 

				    	if (strncmp(buffer,"status",strlen("status"))==0) { 
				    		printf("Lista clientilor conectati:\n");
							for ( j = 0 ; j < nr_clienti ; j ++ ) {
								if (clienti[j].activ==1) {
									printf("Nume %s IP %s Port %d\n",clienti[j].nume,clienti[i].ip,clienti[j].port);
								}
							} 
						}
						if(strncmp(buffer,"kick",strlen("kick")) == 0){
							mesaj.type = 22;
							char *p;
							p = strtok(buffer," ");
							p = strtok(0,"\n");
							char nume[20];
							strcpy(nume,p);
							for ( j = 0 ; j < nr_clienti ; j ++ ) {
								if ((clienti[j].activ==1) && (strcmp(clienti[j].nume,nume)==0)) {
									clienti[j].activ=0;
									send(clienti[j].socket,&mesaj,sizeof(mesaj), 0);
									break;
								}
							} 
						}
						if(strncmp(buffer,"quit",strlen("quit"))==0){ 
							mesaj.type = 22;
							se_inchide = 1 ;
							for ( j = 0 ; j < nr_clienti ; j ++ ) {
								if (clienti[j].activ==1) {
									clienti[j].activ=0;
									send(clienti[j].socket,&mesaj,sizeof(mesaj), 0);
								}
							} 
						}
					}
					else {
						memset(buffer, 0, BUFLEN);
						if ((n = recv(i, &mesaj, sizeof(mesaj), 0)) <= 0) {
							if (n == 0) {
								printf("selectserver: socket %d hung up\n", i);
							} else {
								error("ERROR in recv");
							}
							close(i); 
							FD_CLR(i, &read_fds); 
						} else { 
							switch ( mesaj.type) {
								case 0 : {  
									j = 0 ;
									for ( j = 0 ; j < nr_clienti ; j ++) {
										if ( strcmp(clienti[j].nume,mesaj.payload)==0 ) {
											printf("%s\n",clienti[j].nume);
											printf("selectserver: socket %d hung up\n", i);
											close(i); 
											FD_CLR(i, &read_fds); 
											break ;
										}
									}
									if ( j == nr_clienti) { 
										clienti[nr_clienti].activ = 1 ;
										memcpy(clienti[nr_clienti].nume,mesaj.payload,sizeof(mesaj.payload));
										clienti[nr_clienti].timp = time(NULL) ;
										clienti[nr_clienti].port = mesaj.client.port ;
										memset(clienti[nr_clienti].ip,0,sizeof(clienti[nr_clienti].ip));
										memcpy(clienti[nr_clienti].ip,inet_ntoa(cli_addr.sin_addr),strlen(inet_ntoa(cli_addr.sin_addr))) ;
										clienti[nr_clienti].socket = i ;
										nr_clienti ++ ; 
									} 
									break ;
								}
								case 1 : {
									mesaj.type = 1 ;
									k = 0 ;
									printf("%d\n",nr_clienti);
									sprintf(mesaj.payload," ");
									for ( j = 0 ; j < nr_clienti ; j ++ ) {
										if (clienti[j].activ==1)
											sprintf(mesaj.payload,"%s %s",mesaj.payload,clienti[j].nume);
									}  
									send(i,&mesaj,sizeof(mesaj), 0);
									break ;
								}
								case 2 : {
									mesaj.type = 2 ; 
									time_t timp_aux = time(NULL);
									for ( j = 0 ; j < nr_clienti ; j ++ ) {
										if ((strcmp(clienti[j].nume,mesaj.payload)==0) && (clienti[j].activ==1)) {
											time_t dif = timp_aux - clienti[j].timp;
  											struct tm * t;
  											t = localtime (&dif);
											sprintf(mesaj.payload,"Nume %s Port %d Timp %d:%d:%d\n",clienti[j].nume,clienti[j].port,t->tm_hour,t->tm_min,t->tm_sec);
											break ;
										}
									} 
									if ( j < nr_clienti )
										send(i,&mesaj,sizeof(mesaj), 0);
									break ;
								}
								case 3 : { 
									mesaj.type = 31 ;
									for ( j = 0 ; j < nr_clienti ; j ++ ) {
										if ((strcmp(clienti[j].nume, mesaj.destinatie)==0) && (clienti[j].activ==1)) {
											memcpy(mesaj.client.ip,clienti[j].ip,sizeof(clienti[j].ip)) ;
											memcpy(mesaj.client.nume,clienti[j].nume,sizeof(clienti[j].nume)) ;
											mesaj.client.port=clienti[j].port ;
											mesaj.client.socket=clienti[j].socket ;
											break ;
										}
									} 
									mesaj.timp = time( NULL ) ; 
									if ( j < nr_clienti )
										send(i,&mesaj,sizeof(mesaj), 0);
									break ;
								}
								case 4 : { 
									mesaj.type = 31 ;
									for ( j = 0 ; j < nr_clienti ; j ++ ) {
										if ((strcmp(clienti[j].nume, mesaj.sursa)!=0) && (clienti[j].activ==1)) {
											memcpy(mesaj.client.ip,clienti[j].ip,sizeof(clienti[j].ip)) ;
											memcpy(mesaj.client.nume,clienti[j].nume,sizeof(clienti[j].nume)) ;
											mesaj.client.port=clienti[j].port ;
											mesaj.client.socket=clienti[j].socket ;
											mesaj.timp = time( NULL ) ; 
											send(i,&mesaj,sizeof(mesaj), 0);
										}
									} 
									break ;
								}
								case 5 : { 
									mesaj.type = 51 ;
									for ( j = 0 ; j < nr_clienti ; j ++ ) {
										if ((strcmp(clienti[j].nume, mesaj.destinatie)==0) && (clienti[j].activ==1)) {
											memcpy(mesaj.client.ip,clienti[j].ip,sizeof(clienti[j].ip)) ;
											memcpy(mesaj.client.nume,clienti[j].nume,sizeof(clienti[j].nume)) ;
											mesaj.client.port=clienti[j].port ;
											mesaj.client.socket=clienti[j].socket ;
											break ;
										}
									} 
									if ( j < nr_clienti )
										send(i,&mesaj,sizeof(mesaj), 0);
									break ;
								}
								case 6 : {
									for ( j = 0 ; j < nr_clienti ; j ++) {
										if ( strcmp(clienti[j].nume,mesaj.payload)==0 ) {
											printf("%s\n",clienti[j].nume);
											clienti[j].activ=0;
											printf("selectserver: socket %d hung up\n", i);
											close(i); 
											FD_CLR(i, &read_fds); 
											break ;
										}
									}
								}
							}
						}
					}
				}
			}
			if (se_inchide==1){
				for(i = 0; i <= fdmax; i++) 
					if(FD_ISSET(i, &tmp_fds)) {
						close(i);
						FD_CLR(i, &read_fds); 
					}
				exit(0);
			}
		}

     close(sockfd);
   
     return 0; 
}