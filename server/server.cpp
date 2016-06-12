#include <arpa/inet.h>
#include "header.h"

/* filesynd <port> <thread_pool_size> <queue_size> <tree_height> <tree_fanout>  */
/*	<port> = port number for tcp connections       */
/*	<thread_pool_size> = working threads number    */
/*	<queue_size> = size of communicating queue     */
/*	<tree_height> = static height of the Hash-Tree */
/*	<tree_fanout> = Hash-Tree fan out	       */

int tree_height;
int fan_out;
int port;
pthread_mutex_t cmtx = PTHREAD_MUTEX_INITIALIZER;	/* Mutex for the thread pool queue( Producer-Concumer ) */
pthread_mutex_t socket_mtx = PTHREAD_MUTEX_INITIALIZER;	/* Mutex for internall socket read-write */
pthread_mutex_t list_mtx;
pthread_cond_t cvar;					/* Condition Var for correct (Producer-Concumer) in thread pool*/

int main(int argc, char* argv[]){

	int err;
	if( argc != 6 ){
		cout<<"Wrong Input!!\nRight Format is \"./filesynd <port> <thread_pool_size> <queue_size> <tree_height> <tree_fanout>\""<<endl;
		exit(1);
	}

	port = atoi(argv[1]);
	if( port < 1 ){
		cout<<"Wrong port Number!"<<endl;
		exit(1);
	}

	int wthreads = atoi(argv[2]);
	if( wthreads < 1){
                cout<<"Wrong thread_pool_size!"<<endl;
                exit(1);
        }

	int queue_size = atoi(argv[3]);
	if( queue_size < 1 ){
                cout<<"Wrong queue_size!"<<endl;
                exit(1);
        }

	tree_height = atoi(argv[4]);
        if( tree_height < 1 ){
                cout<<"Wrong tree_height!"<<endl;
                exit(1);
        }
	fan_out = atoi(argv[5]);
        if( fan_out < 1 ){
                cout<<"Wrong fan_out!"<<endl;
                exit(1);
        }

	/* Initialize Queue-List*/
        ListS *list;
        list = new ListS();

        /* Initialize mutex */
        pthread_mutex_init(&list_mtx, NULL);

        /* Initialize condition variables */
        pthread_cond_init(&cvar, NULL);

        /* Allocating Working Threads */
        pthread_t* thr;
        thr = new pthread_t[ wthreads ];

        /* Creating Working Threads */
        for(int i=0;  i < wthreads;  i++){
                if( err = pthread_create( thr+i, NULL, working_thread, (void*) list) ){
                        perror2("pthread_create", err);
                        exit(1);
                }
        }



/* ------------------------------------------ */
/*  Internet stream sockets TCP server
/* ------------------------------------------ */
	int sock, newsock;
	struct sockaddr_in server, client;
	socklen_t clientlen;
	struct sockaddr *serverptr=(struct sockaddr *)&server;
	struct sockaddr *clientptr=(struct sockaddr *)&client;
	struct hostent *rem;
	fd_set active_fd_set, read_fd_set;	//for select

	/* Create socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit((char*)"creating socket error");

        server.sin_family = AF_INET;       		/* Internet domain */
        server.sin_addr.s_addr = htonl(INADDR_ANY);	/* IPv4 address */
        server.sin_port = htons(port);      		/* The given port */

	/* Bind socket to address */
	if (bind(sock, serverptr, sizeof(server)) < 0)
		perror_exit((char*)"binding error");

	/* Listen for connections */
	if (listen(sock, 15) < 0)
		perror_exit((char*)"listening for connections error");

	cout<<"Listening for connections to port : "<<port<<endl;

       /* Initialize the set of active sockets. */
       FD_ZERO (&active_fd_set);
       FD_SET (sock, &active_fd_set);

	int counter=0;
	int i;
        while (1) {
		read_fd_set = active_fd_set;
//		cout<<"Select is Waiting Request.."<<endl;
		/* WE CALL SELECT HERE */
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0){
			perror ("select");
               		exit(EXIT_FAILURE);
            	}
		for (i = 0; i < FD_SETSIZE; ++i){
			if (FD_ISSET (i, &read_fd_set) ){
				if (i == sock){
                     		/* Connection request on original socket. */
                     		int newsock;
                   		size_t size = sizeof (client);
                     		newsock = accept (sock, (struct sockaddr *) &client, &size);
                     			if (newsock < 0){
                         			perror ("accept");
                         			exit (EXIT_FAILURE);
                       			}
					cout<<"Accepted Connection"<<endl;
					FD_SET (newsock, &active_fd_set);
					counter++;
				}
				else{
					/* down ehre i check incomming ip adress in order to be sure where i will send my messages */
					/* that was a bit hard to find .. ^^ */
					struct sockaddr_in clientip;			//i create a temporary sockaddr_in to obtain my ip adress
					socklen_t addr_len;
					clientip.sin_family = AF_INET;
					addr_len = sizeof(clientip)*10;
					getsockname (i, (struct sockaddr *)&clientip, &addr_len);	//pick ip adrres etc..
					string ip = inet_ntoa(clientip.sin_addr);	//transform to string
					/* EXISTING INTERNAL SOCKET MESSAGES */
					if( ip.compare("127.0.0.1") == 0){
						ssize_t recvd;
						struct internal_socket_message msg;
						recvd = recv(i, &msg ,sizeof( struct internal_socket_message ), MSG_WAITALL);
						if(recvd == 0){
							close (i);
							FD_CLR (i, &active_fd_set);
						}
						else if(recvd < 0){
						//	perror ("recv");
						//	exit (EXIT_FAILURE);
						}
						else{
							cout<<"Internal socket message received form ip : "<<ip<<endl;
							/* so lets find out what kind of message we received */
							if( msg.expect > 0 ){
								/* hmmm here we received EXPECT <int> */
								cout<<"EXPECT : "<<msg.expect<<endl;
								/* So we send to our client message EXPECT */
								char message[10];
								strcpy(message,"EXPECT");
								send(msg.client_id, message, sizeof(char)*10, 0);

								/* Now we can send expect number to our client because he knows what to expect */
								int expect = msg.expect;
								send(msg.client_id, &expect, sizeof(int), 0);
							}
							else if (msg.expect < 0 ){
								/* And here we have FILEINFO size,filename,hash tree etc */
								char message[10];
								/* send FILEINFO */
								strcpy(message,"FILEINFO");
								send(msg.client_id, message, sizeof(char)*10, 0);
 								/* send FILE NAME */
								send(msg.client_id, msg.filename, sizeof(char)*30, 0);
                                                                /* send FILE SIZE */
                                                                send(msg.client_id, &(msg.size),sizeof(int), 0);
								/* send HASH TREE */
								struct Encoded* tree = msg.tree_pointer;	//we passed before pointer to speed up */
								int nodes =( power(fan_out, tree_height)-1)/(fan_out-1);
								send(msg.client_id, tree, sizeof(struct Encoded)*nodes, 0);
							}
							else{
								/* DATA filename, pagenumber, pagedata etc */
								struct DATA_message client_msg;
								client_msg.die = 0;
								strcpy( client_msg.filename, msg.filename);
								client_msg.pagenum = msg.page_number;
//								strcpy( client_msg.page_data, msg.page_data);
								send(msg.client_id, &client_msg, sizeof(struct DATA_message), 0);
								send(msg.client_id, msg.page_data, PAGESIZE, 0);

							}
						}//if nbytes >=0
					}
					/* EXISTING EXTERNAL SOCKET MESSAGES */
					else{
        					char* buff;
				        	buff = new char[100];
						int nbytes;
//						nbytes = read(i, buff, 100 );cout<<"bytes = "<<nbytes<<endl;
						nbytes = recv(i, buff ,100, MSG_WAITALL);
                                                if( nbytes == 0 ){
                                                        close (i);
                                                        FD_CLR (i, &active_fd_set);
                                                }
                                                else if( nbytes < 0 ){
  //                                                      perror ("read");
//                                                        exit (EXIT_FAILURE);
						}
						else{
	                                                string message_c( buff );
							if( message_c[0] == '0' ){	/* here if our client finished so goodbye */
								cout<<"Goodbye Client : "<<ip<<endl;
								struct DATA_message client_msg;
								client_msg.die = 1;
								send(i, &client_msg, sizeof(struct DATA_message), 0);
							}
							else{
        	                                        	cout<<"External Socket Message : "<<message_c<<" form ip : "<<ip<<endl;
       	       	 	                                	inserting_client_message_working_thread_list(message_c,list,i);
							}
						}
					}
					//counter++;
				}
			}
		}
	}


        for(int i=0;  i < wthreads;  i++){
                if( err = pthread_join( thr[i], NULL ) ){                                       //w8 for threads termination
                        perror2("pthread_join", err);
                        exit(1);
                }
        }

return 0;
}


/* ---------------------- */
/* SERVER WORKING THREADS */
/* ---------------------- */

void* working_thread(void* lists){
	bool Connected = false;
	int err;
        int sock;
        struct sockaddr_in server;
        struct sockaddr *serverptr = (struct sockaddr*)&server;
        struct hostent *rem;
        /* Create Internal Socket */
        if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                perror_exit((char*)"socket");
        /* Find server address */
        if ((rem = gethostbyname( "127.0.0.1" ) ) == NULL) {
                herror("gethostbyname");
               exit(1);
        }
        server.sin_family = AF_INET;       /* Internet domain */
        memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
        server.sin_port = htons(port);         /* Server port */

	while(1){
                ListS* list = (ListS*) lists;
                                                                                /* CONSUMER */
                        /* Lock condition mutex */
                        if( err = pthread_mutex_lock(&cmtx) ){                  /* locking mutex for the condition var */
                                perror2("pthread_mutex_lock", err);
                                exit(1);
                        }

                        /*Lock List Mutex */
                         if( err = pthread_mutex_lock(&list_mtx) ){             /* locking for list inserting */
                                perror2("pthread_mutex_lock", err);
                                exit(1);
                        }
                        while( list -> size() == 0){
                                /*Unlocking list mutex so when the current thread w8 the main thread to be able to insert */
                                if( err = pthread_mutex_unlock(&list_mtx) ){
                                        perror2("pthread_mutex_lock", err);
                                        exit(1);
                                }

//                                cout<<pthread_self()<<" W8ing for signal "<<endl;

                        /* --------------------->  Waiting for the signal  <------------------------*/
                                pthread_cond_wait(&cvar, &cmtx);

                                if( err = pthread_mutex_lock(&list_mtx) ){
                                        perror2("pthread_mutex_lock", err);
                                        exit(1);
                                }
                        /* Return to the loop now with locked mutex( for while's condition ) */
                        }

		         /* here we already have the mutex locked from the loop so we are ok with the out() */
                        DataS temp = list -> out();
			string message_c = temp.message_c;
	                string word[5];
                	stringstream ss(message_c);
                       // cout<<pthread_self()<<" removed : "<<temp.message_c<<endl;

                        if( err = pthread_mutex_unlock(&list_mtx)){
                                perror2("pthread_mutex_unlock", err);
                                exit(1);
                        }
                        /*Unlock mutexes */
                        if( err = pthread_mutex_unlock(&cmtx)){
                                perror2("pthread_mutex_unlock", err);
                                exit(1);
                        }

	                /* here i will check what kind of message our client sent (its only for 2 or 4 word messages) */
        	        int i = 0;
        	        while (ss.good()) {
                	        if( i == 5 )
                       	        	cout<<"Error! too big message from client."<<endl;
                      		ss >> word[i];
                      		i++;
                	}
                	if(i == 2){
                        	if( word[0].compare("GETFILES") == 0 ){
					/* Find All files in the word[1] directory and insert PROCESSFILE message to the thread pool for each one */
					cout<<pthread_self()<<" received GETFILES message"<<endl;

        				DIR *dir;
        				struct dirent *ent;
        				dir = opendir(word[1].c_str());
        				int file_number = 0 ;
        				if( dir == NULL ){
                				cout<<"Failed to read directory '"<<word[1]<<"'"<<endl;
                				exit(1);
        				}
        				else{
                				while ((ent = readdir (dir)) != NULL){
                        				if( file_number != 0 && file_number != 1){
                                				string name1(ent->d_name);
                                									/* PRODUCER */
								DataS tmp;
								tmp.client_id = temp.client_id;
								tmp.message_c = "PROCESSFILE "+ word[1]+" "+name1;

			                			/* Lock condition mutex */
                						if( err = pthread_mutex_lock(&cmtx) ){
                        						perror2("pthread_mutex_lock", err);
                        						exit(1);
                						}
								/* Lock List Mutex */
                						if( err = pthread_mutex_lock(&list_mtx) ){
                        						perror2("pthread_mutex_lock", err);
                        						exit(1);
                						}

                						list -> insert(tmp);

                						/* Unlock List Mutex */
                						if( err = pthread_mutex_unlock(&list_mtx) ){
                 	       						perror2("pthread_mutex_lock", err);
                			        			exit(1);
                						}

		 				                pthread_cond_broadcast( &cvar);

                						/* Unlock condition mutex */
                						if( err = pthread_mutex_unlock(&cmtx) ){
                        						perror2("pthread_mutex_lock", err);
                        						exit(1);
								}
							}
						file_number++;
						}//while reading filenames
						/* Send here file_number to main thread 'EXPECT' */

							if(Connected == false){
								if(connect(sock, serverptr, sizeof(server)) <0)
									perror_exit((char*)"connect");
								Connected = true;
							}
						struct internal_socket_message msg;
						msg.client_id = temp.client_id;
						msg.expect = file_number-2;
						strcpy(msg.filename,"NULL");
						send(sock, &msg, sizeof( struct internal_socket_message) , 0);
//						write(sock, &msg, sizeof( struct internal_socket_message) );
					}
				}//in GETFILES
                        }
			else if( i == 3 ){
				/* If we recieve PROCESSFILE <directory> <filename> */
				if( word[0].compare("PROCESSFILE")==0 ){
					cout<<pthread_self()<<" received PROCESSFILE message"<<endl;

			                struct Encoded** level;
        			        string str = word[1]+"/"+word[2];
        			        level = TreeCreation(tree_height, fan_out, (char*)str.c_str());

					/*Allocating and creating 1D Hash Tree Array*/
        				struct Encoded* new_level;
					Transform2D_to_1D(tree_height, fan_out, level, new_level);

				/* So now we have created our hash tree we will send it via internal socket to our Main thread */
        			if(Connected == false){
                			if(connect(sock, serverptr, sizeof(server)) <0)
                 		        	perror_exit((char*)"connect");
        			        Connected = true;
        			}
				/* Creating FILEINFO message and sending it */
                                struct internal_socket_message msg;
                                msg.client_id = temp.client_id;		/* client id*/
                                msg.expect = -1;			/* identifier for FILEINFO */
				msg.tree_pointer = new_level;		/* Hash tree */
				strcpy( msg.filename , word[2].c_str());/* filename */
				struct stat st;
				stat(str.c_str(), &st);
				int size = st.st_size;			/* filesize */
				msg.size = size;

				send(sock, &msg, sizeof( struct internal_socket_message) , 0);
//                                write(sock, &msg, sizeof( struct internal_socket_message) );

				}
			}
                        else if( i == 5 ){
				/* GETDATA folder filename PageNo */
                                if(word[0].compare("GETDATA")==0 ){
					cout<<pthread_self()<<" received GETDATA message"<<endl;
					struct internal_socket_message msg;
					msg.client_id = temp.client_id;		/* client id*/
					msg.expect = 0;				/* Identifier for DATA */
					strcpy( msg.filename , word[2].c_str());/* File Name */
					msg.page_number = atoi(word[4].c_str());/* Page Number */
					/* and now lets find the page_data */

					string path = word[1] +"/"+word[2];
					FILE *fp;
					fp = fopen(path.c_str(),"r+");
					fseek(fp, msg.page_number * PAGESIZE, SEEK_SET);
					struct stat st;
					stat(path.c_str(), &st);
					int size = st.st_size;
					int mod = size % PAGESIZE;
					fread(msg.page_data,1,PAGESIZE,fp);
					fclose(fp);
					send(sock, &msg, sizeof( struct internal_socket_message) , 0);
				}
                	}
			else
				cout<<"ERROR!--"<<endl;

	}
}
void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void inserting_client_message_working_thread_list(string message_c, ListS*& list,int socket_fd){
		int err;

                /* Lock condition mutex */
                if( err = pthread_mutex_lock(&cmtx) ){
                        perror2("pthread_mutex_lock", err);
                        exit(1);
                }
                /* Lock List Mutex */
                if( err = pthread_mutex_lock(&list_mtx) ){
                        perror2("pthread_mutex_lock", err);
                        exit(1);
                }
                DataS tmp;
                tmp.client_id = socket_fd;	//so now we can write back to our client
                tmp.message_c = message_c;

       /* ----> WAKING up sleeping threads<---- */
                list -> insert(tmp);

                pthread_cond_broadcast( &cvar);

                /* Unlock List Mutex */
                if( err = pthread_mutex_unlock(&list_mtx) ){
                        perror2("pthread_mutex_lock", err);
                        exit(1);
                }

                /* Unlock condition mutex */
                if( err = pthread_mutex_unlock(&cmtx) ){
                        perror2("pthread_mutex_lock", err);
                        exit(1);
                }

}
