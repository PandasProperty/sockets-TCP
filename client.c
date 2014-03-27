#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "mesaj.h"
#include <fcntl.h>

#define MAX_CLIENTS	100
#define BUFLEN 256

void error(char *msg) {
    perror(msg);
    exit(1);
}

client clienti[MAX_CLIENTS] ;
int nr_clienti = 0 ;

FILE* fisier_history ;

int main(int argc, char *argv[]) {
	char nume_history[30];
	sprintf(nume_history,"%s_history",argv[1]); 
	
	fisier_history=fopen(nume_history,"a+");

    int sockfd, newsockfd, portno, accept_len ;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, listen_addr, accept_addr;
    int n, i, j, k , inchidere = 0 ;

    struct timeval TIMEOUT;
    TIMEOUT.tv_sec = 1000;
    TIMEOUT.tv_usec = 0;

    msg mesaj ;
  
    fd_set read_fds;	
    fd_set tmp_fds;	
    int fdmax;		

    if (argc < 5) {
       fprintf(stderr,"Usage %s client_name server_address server_port\n", argv[0]);
       exit(0);
    } 

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    portno = atoi(argv[4]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr));

	int listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset((char *) &listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(atoi(argv[2]));			
	listen_addr.sin_addr.s_addr = INADDR_ANY;	
	bind(listen_sockfd, (struct sockaddr *) &listen_addr, sizeof(struct sockaddr)) ;
	listen(listen_sockfd, MAX_CLIENTS);

	socklen_t listen_len = sizeof(listen_addr);
	getsockname(listen_sockfd, (struct sockaddr *) &listen_addr, &listen_len);

	mesaj.type = 0 ;
	mesaj.client.port = atoi(argv[2]); 
    memcpy(mesaj.payload,argv[1],sizeof(argv[1])); 
    send(sockfd,&mesaj,sizeof(mesaj),0);

	FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
   
    FD_SET(listen_sockfd, &read_fds);
	fdmax = listen_sockfd;

	FD_SET(fileno(stdin), &read_fds);
	FD_SET(sockfd, &read_fds);
	
	int f, g;
	int contor_fisier = 0 ;
	int mai_am_de_citit = 0 ;
	msg mesaj_de_trimis ;

    while (1) { 
		tmp_fds = read_fds; 
		if ( select(fdmax + 1, &tmp_fds, NULL, NULL, &TIMEOUT) >= 0 ){
			for(i = 0; i <= fdmax; i++) {
				if(FD_ISSET(i, &tmp_fds)) {
					if ( i == 0 ) { 
			    		memset(buffer, 0 , BUFLEN);
			    		fgets(buffer, BUFLEN-1, stdin); 

				    	if(strncmp(buffer,"listclients",strlen("listclients"))==0){ 
							mesaj.type = 1;
							send(sockfd,&mesaj,sizeof(mesaj), 0);   
						}
						if(strncmp(buffer,"infoclient",strlen("infoclient")) == 0){
							mesaj.type = 2;
							char *p;
							p = strtok(buffer," ");
							p = strtok(0,"\n");
							strcpy(mesaj.payload,p);
							send(sockfd,&mesaj,sizeof(mesaj), 0); 
						}
						if(strncmp(buffer,"message",strlen("message")) == 0){ 
							char *p;
							p = strtok(buffer," ");
							p = strtok(0," ");
							memcpy(mesaj.sursa,argv[1],sizeof(argv[1]));
							strcpy(mesaj.destinatie,p); 
							p = strtok(0,"\n"); 
							memset(mesaj.payload,0,sizeof(mesaj.payload));
							strncpy(mesaj.payload,p,sizeof(mesaj.payload)-1);
							mesaj.type = 3; 
							send(sockfd,&mesaj,sizeof(mesaj), 0); 
						}
						if(strncmp(buffer,"broadcast",strlen("broadcast")) == 0){
							char *p;
							p = strtok(buffer," ");
							p = strtok(0,"\n"); 
							memset(mesaj.payload,0,sizeof(mesaj.payload));
							strncpy(mesaj.payload,p,sizeof(mesaj.payload)-1);
							mesaj.type = 4;
							memcpy(mesaj.sursa,argv[1],sizeof(argv[1])); 
							send(sockfd,&mesaj,sizeof(mesaj), 0); 
						}
						if(strncmp(buffer,"sendfile",strlen("sendfile")) == 0){
							char *p;
							p = strtok(buffer," ");
							p = strtok(0," ");
							strcpy(mesaj.destinatie,p); 
							p = strtok(0,"\n");
							strcpy(mesaj.nume_fisier,p); 
							mesaj.type = 5;
							send(sockfd,&mesaj,sizeof(mesaj), 0); 
						}
						if (strncmp(buffer,"history",strlen("history"))==0){
							char *string = NULL;
							int c;
							size_t len = 0;
							rewind(fisier_history);
							while ((c=getline(&string, &len, fisier_history))>0) {
								printf("%s",string);
							}
						}
						if (strncmp(buffer,"quit",strlen("quit"))==0){
							inchidere = 1 ;
							mesaj.type = 6;
							memcpy(mesaj.payload,argv[1],sizeof(argv[1]));
							send(sockfd,&mesaj,sizeof(mesaj), 0); 
						}
			    	} else 
		    		if ( i == sockfd ) {
						if ((recv(sockfd, &mesaj, sizeof(mesaj), 0))==0) {
							close(i);
							FD_CLR(i, &read_fds); 
						}
						switch ( mesaj.type ) {
					    	case 1 : {
					    		printf("List of Clients:%s\n", mesaj.payload );
					    		break ;
					    	}
					    	case 2 : {
					    		printf("infoclient : %s", mesaj.payload );
					    		break ;
					    	}
					    	case 22 : {
					    		inchidere = 1 ;
					    	}
					    	case 31 : {  
					    		mesaj.type = 3 ; 

					    		int cli2_sockfd = socket(AF_INET, SOCK_STREAM, 0);
								
								struct sockaddr_in client2_addr;
								memset((char *) &client2_addr, 0, sizeof(client2_addr));
								client2_addr.sin_family = AF_INET;
								client2_addr.sin_port = htons(mesaj.client.port);
								inet_aton(mesaj.client.ip, &client2_addr.sin_addr);		
								connect(cli2_sockfd, (struct sockaddr*) &client2_addr, sizeof(client2_addr));
								send(cli2_sockfd, &mesaj, sizeof(mesaj), 0);

								break ;
					    	}
					    	case 51: {
					    		memcpy(&mesaj_de_trimis,&mesaj,sizeof(mesaj)); 
					    		int cli2_sockfd = socket(AF_INET, SOCK_STREAM, 0);
								
								struct sockaddr_in client2_addr;
								memset((char *) &client2_addr, 0, sizeof(client2_addr));
								client2_addr.sin_family = AF_INET;
								client2_addr.sin_port = htons(mesaj.client.port);
								inet_aton(mesaj.client.ip, &client2_addr.sin_addr);		
								connect(cli2_sockfd, (struct sockaddr*) &client2_addr, sizeof(client2_addr));

								f = open( mesaj.nume_fisier, O_RDONLY) ;
								mai_am_de_citit = 1 ;
								sprintf(mesaj.nume_fisier,"%s_primit",mesaj.nume_fisier);

								mesaj.type = 52 ; 
								sprintf(mesaj.payload,"Incepe copierea fisierului %s",mesaj.nume_fisier);
								send(cli2_sockfd, &mesaj, sizeof(mesaj), 0);

								mesaj.type = 51 ;
								int contor ; 
								memset( mesaj.payload,0,sizeof(mesaj.payload));
								while ( (contor = read( f , mesaj.payload , 1024)) > 0 ) {
									mesaj.len = contor ;
									send(cli2_sockfd, &mesaj, sizeof(mesaj), 0);
									memset( mesaj.payload,0,sizeof(mesaj.payload));
								}
								close(f);
								mai_am_de_citit = 0 ;

								mesaj.type = 53 ; 
								sprintf(mesaj.payload,"S-a primit fisierul %s",mesaj.nume_fisier);
								send(cli2_sockfd, &mesaj, sizeof(mesaj), 0);

								break ;
					    	}
					    }
				    } else
					if ( i == listen_sockfd ) {
						accept_len = sizeof(accept_addr);
						newsockfd = accept(listen_sockfd, (struct sockaddr *)&accept_addr, (socklen_t *)&accept_len) ;
						
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) {
							fdmax = newsockfd;
						}
					}
					else { 
						if ( recv(i, &mesaj, sizeof(mesaj), 0) == 0 ) {
							close(i);
							FD_CLR(i, &read_fds); 
						}
						switch ( mesaj.type ){
							case 3 : { 
								time_t dif = time(NULL) - mesaj.timp;
  								struct tm * t;
  								t = localtime (&dif);
					    		printf("timp: %d:%d:%d , sursa: %s , mesaj: %s\n",t->tm_hour,t->tm_min,t->tm_sec, mesaj.sursa, mesaj.payload );
					    		fprintf(fisier_history,"%s\n",mesaj.payload);
					    		break ;
					    	}
					    	case 52 : {
					    		g = open( mesaj.nume_fisier , O_CREAT | O_WRONLY | O_TRUNC, 0644 ) ;
					    		printf("%s\n",mesaj.payload); 
					    		break;
					    	}
					    	case 51 : { 
					    		write(g,mesaj.payload,mesaj.len);
					    		break;
					    	}
					    	case 53 : {
					    		fprintf(fisier_history,"%s\n",mesaj.nume_fisier);
					    		close(g);
					    		printf("%s\n",mesaj.nume_fisier);
					    		break;
					    	}
						}
					}
				}
				if (inchidere==1){
					for(i = 0; i <= fdmax; i++) 
						if(FD_ISSET(i, &tmp_fds)) {
							close(i);
							FD_CLR(i, &read_fds); 
						}
					exit(0);
				}
			} 
		} 
		else { 
			if ( mai_am_de_citit == 1 ) {
				int cli2_sockfd = socket(AF_INET, SOCK_STREAM, 0);
				struct sockaddr_in client2_addr;
				memset((char *) &client2_addr, 0, sizeof(client2_addr));
				client2_addr.sin_family = AF_INET;
				client2_addr.sin_port = htons(mesaj_de_trimis.client.port);
				inet_aton(mesaj_de_trimis.client.ip, &client2_addr.sin_addr);		
				connect(cli2_sockfd, (struct sockaddr*) &client2_addr, sizeof(client2_addr));

				f = open( mesaj_de_trimis.nume_fisier, O_RDONLY) ;
				mai_am_de_citit = 0 ;
				sprintf(mesaj_de_trimis.nume_fisier,"%s_primit",mesaj_de_trimis.nume_fisier);

				mesaj_de_trimis.type = 51 ;
				int contor ; 
				memset( mesaj_de_trimis.payload,0,sizeof(mesaj_de_trimis.payload));
				while ( (contor = read( f , mesaj_de_trimis.payload , 1024)) > 0 ) {
					mesaj_de_trimis.len = contor ;
					send(cli2_sockfd, &mesaj_de_trimis, sizeof(mesaj_de_trimis), 0);
					memset( mesaj_de_trimis.payload,0,sizeof(mesaj_de_trimis.payload));
				}
				close(f);
				mai_am_de_citit = 0 ;

				mesaj_de_trimis.type = 53 ; 
				sprintf(mesaj_de_trimis.payload,"S-a primit fisierul %s",mesaj_de_trimis.nume_fisier);
				send(cli2_sockfd, &mesaj_de_trimis, sizeof(mesaj_de_trimis), 0);
			} 
		} 
	}

	fclose(fisier_history);	
    close(sockfd);
   
    return 0; 
}