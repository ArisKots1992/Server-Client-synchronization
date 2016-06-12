#include "header.h"

/* filesync <server_ip> <server_port> <thread_pool_size> <queue_size> <tree_height> <tree_fanout> <directory_name>  */
/*	<server_ip> = the IP adress that server uses   */
/*      <server_port> = serversport number for tcp     */
/*      <thread_pool_size> = working threads number    */
/*      <queue_size> = size of communicating queue     */
/*      <tree_height> = static height of the Hash-Tree */
/*      <tree_fanout> = Hash-Tree fan out              */
/*	<directory_name> = directory for sync	       */

/* CLIENT MAIN THREAD */

pthread_mutex_t list_mtx,list_mtx1,term;		/* Mutex for using queue correctly in threads */
pthread_mutex_t cmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t end_client = PTHREAD_MUTEX_INITIALIZER;	/* to temrinate client*/
pthread_mutex_t cmtx1 = PTHREAD_MUTEX_INITIALIZER; 	/* For Consumer-producer 1 for queue and 1 for the sorted list */
pthread_cond_t cvar,cvar1;				/* Condition variable */
bool Terminate_WTH = false;
int tree_height;
int fan_out;
string directory;
/* Initialize Sorted Hash Tree List */
ListHash *listH;
string final;

int main(int argc, char* argv[]){

	/* ------------------------------------------------ */
	/* Command like entries check & initiating variable */
        /* ------------------------------------------------ */
	int err;
	if( argc != 8 ){
		cout<<"Wrong Format!"<<endl<<"Correct Format : ";
		cout<<"filesync <server_ip> <server_port> <thread_pool_size> <queue_size> <tree_height> <tree_fanout> <directory_name>"<<endl;
		exit(1);
	}

	string server_ip = argv[1];

	int port = atoi(argv[2]);
	if( port < 1 ){
		cout<<"Wrong server port!"<<endl;
		exit(1);
	}

	int wthreads = atoi(argv[3]);
        if( wthreads < 1 ){
                cout<<"Wrong thread pool size!"<<endl;
                exit(1);
        }

	int queue_size = atoi(argv[4]);
	if( queue_size < 1 ){
                cout<<"Wrong queue size!"<<endl;
                exit(1);
        }

	tree_height = atoi(argv[5]);
        if( tree_height < 1 ){
                cout<<"Wrong tree height!"<<endl;
                exit(1);
        }

	fan_out = atoi(argv[6]);
        if( fan_out < 1 ){
                cout<<"Wrong fan out!"<<endl;
                exit(1);
        }
	directory = argv[7];

	/* Initialize Queue and Sorted List*/
	ListC *list;
	list = new ListC();
	listH = new ListHash();

	/* Initialize mutex */
	pthread_mutex_init(&list_mtx, NULL);
	pthread_mutex_init(&list_mtx1, NULL);
	pthread_mutex_init(&term, NULL);

	/* Initialize condition variables */
	pthread_cond_init(&cvar, NULL);
	pthread_cond_init(&cvar1, NULL);

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
        /* ----------------------------------------------- */
	/* Reading directory and list inserting messages */
	/* ----------------------------------------------- */
	DIR *dir;
	struct dirent *ent;
	dir = opendir(directory.c_str());
	int file_number = 0 ;
	if( dir == NULL ){
		cout<<"Failed to read directory '"<<directory<<"'"<<endl;
		Terminate_WTH = true;
		pthread_cond_broadcast( &cvar);
		exit(1);
	}
	else{
		while ((ent = readdir (dir)) != NULL){
			if( file_number != 0 && file_number != 1){
				string name1(ent->d_name);
				string name="CALCULATE_HASH "+name1;	/* PRODUCER */
                        /* Lock condition mutex */
                        if( err = pthread_mutex_lock(&cmtx) ){
                                perror2("pthread_mutex_lock", err);
                                exit(1);
                        }
			                /*Lock List Mutex */
                		if( err = pthread_mutex_lock(&list_mtx) ){
                        		perror2("pthread_mutex_lock", err);
                        		exit(1);
                		}

				list -> insert(name);
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
        	}
        closedir(dir);
    	}
	file_number-=2;
	cout<<"Number of Files : "<<file_number<<endl;
	/* --------------------------------------------------------------------------------- */
	/* Now we will conect to our server via TCP for first time and sent GETFILES message */
        /* --------------------------------------------------------------------------------- */
	ListHash *ListServer; 			/* here we will put hash trees from servers directory */
	ListServer = new ListHash();
	int sock;
	struct sockaddr_in server;
	struct sockaddr *serverptr = (struct sockaddr*)&server;
	struct hostent *rem;

	/* Create socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit((char*)"socket");

	/* Find server address */
	if ((rem = gethostbyname( server_ip.c_str() ) ) == NULL) {
		herror("gethostbyname");
		exit(1);
	}

	server.sin_family = AF_INET;       /* Internet domain */
	memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
	server.sin_port = htons(port);         /* Server port */

	/* Initiate connection */
	if (connect(sock, serverptr, sizeof(server)) < 0)
		perror_exit((char*)"connect");

	cout<<"Connecting to "<<server_ip<<" via port "<<port<<endl;

	char *buffer;
	buffer = new char[100];
	string msga = "GETFILES "+directory;
	strcpy(buffer,msga.c_str());
/* Sending GETFILES folder */
//	write(sock, buffer ,100);
	send(sock,buffer,100,0);
/* And now we recieve Expect and FILEINFO */
	ssize_t  recvd;
        int expect = -1;
	int count_fileinfo=0;
	while(1){

		if(expect != -1){
			if( count_fileinfo == expect )
				break;
		}

		char mess[10];
		recvd = recv(sock, mess ,sizeof(char)*10, MSG_WAITALL);
		string message(mess);

		if(message.compare("EXPECT") == 0){
			/* So Now we Know we expect one intiget */
			recvd = recv(sock, &expect ,sizeof(int), MSG_WAITALL);
			//cout<<"EXPECT -> "<<expect<<endl;
		}
		else if(message.compare("FILEINFO") == 0){
			/* recieve filename */
			count_fileinfo++;
			char filename[30];
			recvd = recv(sock, filename ,sizeof(char)*30, MSG_WAITALL);
			//cout<<"FiLeNamE : "<<filename<<endl;
			/* recieve filesize */
			int filesize;
                	recvd = recv(sock, &filesize ,sizeof(int), MSG_WAITALL);
                	//cout<<"FiLeSiZe : "<<filesize<<endl;
			/* recieve hash tree */
			struct Encoded* new_level;
			int nodes =( power(fan_out, tree_height)-1)/(fan_out-1);
			new_level = new struct Encoded[nodes];
			recvd = recv(sock, new_level ,sizeof(struct Encoded)*nodes, MSG_WAITALL);
			struct Encoded** level;
			level = TreeAllocation(tree_height,fan_out);
			Assign1D_to_2D(tree_height,fan_out,level,new_level);
			string name(filename);
			ListServer -> insert(level,name,filesize);
		}
	}



        /* ------------------------------------------------------------------------------- */
	/* Now we suspend main thread in order to finish ALL working the threads remaining */
        /* ------------------------------------------------------------------------------- */
        /*Lock List Mutex */
        if( err = pthread_mutex_lock(&cmtx1) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }

	if( err = pthread_mutex_lock(&list_mtx1) ){
		perror2("pthread_mutex_lock", err);
		exit(1);
 	}
	while(listH->size() != file_number){
		/* unlocked in order to can be inserted */
        	if( err = pthread_mutex_unlock(&list_mtx1) ){
                	perror2("pthread_mutex_lock", err);
                	exit(1);
        	}
		/* waiting */
		pthread_cond_wait(&cvar1, &cmtx1);
		 /* Lock condition mutex */
		if( err = pthread_mutex_lock(&list_mtx1) ){
 			perror2("pthread_mutex_lock", err);
             		exit(1);
		}
	}
        if( err = pthread_mutex_unlock(&list_mtx1) ){
        	perror2("pthread_mutex_lock", err);
       		exit(1);
        }
        if( err = pthread_mutex_unlock(&cmtx1) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }

/* For Correct Terminate WTH */
        if( err = pthread_mutex_lock(&term) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }

	Terminate_WTH = true;

        if( err = pthread_mutex_unlock(&term) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }
	/* Unlock condition mutex and Tell Working Threads to kill themselves */
 	if( err = pthread_mutex_lock(&cmtx) ){
		perror2("pthread_mutex_lock", err);
		exit(1);
	}

	pthread_cond_broadcast( &cvar);
 /* Lock condition mutex */
	if( err = pthread_mutex_unlock(&cmtx) ){
		perror2("pthread_mutex_lock", err);
        	exit(1);
        }


/* Now That we are sure we end working threads we call pthread_join so we go at the next level */
        for(int i=0;  i < wthreads;  i++){
                if( err = pthread_join( thr[i], NULL ) ){					//w8 for threads termination
                        perror2("pthread_join", err);
                        exit(1);
                }
        }
	/* So from Now on we Are SURE that Working threads are dead */
//	cout<<"Client Finished successfully!"<<endl;
	cout<<"All Working Threads Are dead!!"<<endl;
	cout<<"Client Hash Tree List has : "<< listH->size()<<" Elements."<<endl;
	cout<<"Server Hash Tree List has : " << ListServer->size()<<" Elements."<<endl;

/* NOW i will create a thread only for reading and main thread will write everything */
pthread_t reading_thread;

if (err = pthread_create(&reading_thread, NULL, read_th, (void*)sock)){
	perror2("pthread_create", err);
	exit(1);
}

/* so Main thread will now find and send ALL the GETDATA messages to our server */

int target_client = 0;
int source_server = 0;

while ( source_server < ListServer->size() ){
	bool exist = false;
	target_client = 0;
	while( target_client < listH->size() ){
		if( ListServer->name(source_server).compare(listH->name(target_client))== 0 ){
			compare_trees(ListServer->tree(source_server),listH->tree(target_client),listH->name(target_client),0,0,sock);
			exist = true;
		}
		target_client++;
	}
	if( exist == false){
		cout<<"Creating file "<< ListServer->name(source_server) <<" with size "<< ListServer->size(source_server)<<endl;
		FILE *fp;
		string path=directory+"/"+ListServer->name(source_server);
		fp=fopen(path.c_str(),"w");
		fclose(fp);
	}
source_server++;
}
target_client = 0;
source_server = 0;
while ( target_client < listH->size() ){
	bool itsok = false;
	source_server = 0;
	while( source_server < ListServer->size() ){
		if( ListServer->name(source_server).compare(listH->name(target_client)) == 0 ){
			itsok = true;
		}
	source_server++;
	}
	if(itsok == false){
		cout<<"Deleting file "<< listH->name(target_client)<<endl;
		string path = directory+"/"+listH->name(target_client);
		remove(path.c_str());
	}
target_client++;
}
                 /* Lock condition mutex */
   //             if( err = pthread_mutex_lock(&end_client) ){
  //                      perror2("pthread_mutex_lock", err);
 //                       exit(1);
//                }
		condition_w8();
		strcpy(buffer,"0 FINALIZE");
		send(sock, buffer ,100,0);

                if( err = pthread_join( reading_thread, NULL ) ){                                       //w8 for threads termination
                        perror2("pthread_join", err);
                        exit(1);
                }
cout<<"Client Finished Successfully!"<<endl;

return 0;
}

/* ---------------------- */
/* CLIENT WORKING THREADS */
/* ---------------------- */

void* working_thread(void* listc){
	int err;
        if( err = pthread_mutex_lock(&term) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }
	bool Terminate = Terminate_WTH;
        if( err = pthread_mutex_unlock(&term) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }

	while( Terminate == false){

		ListC* list = (ListC*) listc;
										/* CONSUMER */
                        /* Lock condition mutex */
                        if( err = pthread_mutex_lock(&cmtx) ){			/* locking mutex for the condition var */
                                perror2("pthread_mutex_lock", err);
                                exit(1);
                        }

			/*Lock List Mutex */
       	        	 if( err = pthread_mutex_lock(&list_mtx) ){		/* locking for list inserting */
        	                perror2("pthread_mutex_lock", err);
                       		exit(1);
                	}
			while( list -> size() == 0){
				/*Unlocking list mutex so when the current thread w8 the main thread to be able to insert */
		                if( err = pthread_mutex_unlock(&list_mtx) ){
        		                perror2("pthread_mutex_lock", err);
        		                exit(1);
        		        }
				/* Mutex for correct termination */
			        if( err = pthread_mutex_lock(&term) ){
                			perror2("pthread_mutex_lock", err);
                			exit(1);
        			}

				bool Terminate1 = Terminate_WTH;

                                if( err = pthread_mutex_unlock(&term) ){
                                        perror2("pthread_mutex_lock", err);
                                        exit(1);
                                }
				 /* If we finished with that thread unlock and end that thread */
				if( Terminate1 == true ){
                			if( err = pthread_mutex_unlock(&cmtx)){
                        			perror2("pthread_mutex_unlock", err);
                        			exit(1);
                			}
				cout<<"Thread ( "<<pthread_self()<<" ) : Terminated!"<<endl;
				return NULL;
				}
				cout<<pthread_self()<<" W8ing for signal "<<endl;
			/* --------------------->  Waiting for the signal  <------------------------*/

				pthread_cond_wait(&cvar, &cmtx);

				/* Checking again for correct Termination */
                                if( err = pthread_mutex_lock(&term) ){
                                        perror2("pthread_mutex_lock", err);
                                        exit(1);
                                }
                                bool Terminate2 = Terminate_WTH;
                                if( err = pthread_mutex_unlock(&term) ){
                                        perror2("pthread_mutex_lock", err);
                                        exit(1);
				}
				/* if we finished with that thread unlock and end thread */
				if( Terminate2 == true ){
                			if( err = pthread_mutex_unlock(&cmtx)){
        			                perror2("pthread_mutex_unlock", err);
	                		        exit(1);
               				}
				cout<<"Thread ( "<<pthread_self()<<" ) : Terminated!"<<endl;
				return NULL;
				}
				if( err = pthread_mutex_lock(&list_mtx) ){
                        		perror2("pthread_mutex_lock", err);
                        		exit(1);
                		}
			/* Return to the loop now with locked mutex( for while's condition ) */
			}
			/* here we already have the mutex locked from the loop so we are ok with the out() */
			string temp = list -> out();
			int pos = temp.find("CALCULATE_HASH");
			string FileName = temp.substr(pos + strlen("CALCULATE_HASH")+ 1);

			cout<<pthread_self()<<" removed : "<<FileName<<endl;

	                if( err = pthread_mutex_unlock(&list_mtx)){
	                        perror2("pthread_mutex_unlock", err);
        	                exit(1);
        	        }
               		/*Unlock mutexes */
               		if( err = pthread_mutex_unlock(&cmtx)){
                       		perror2("pthread_mutex_unlock", err);
                        	exit(1);
                	}
	/* ------------------------ */
	/* HASH TREE CREATING etc.. */
	/* ------------------------ */
	//WITH THE SLEEP we can check the paralell sleep(3);
        if( err = pthread_mutex_lock(&cmtx1) ){			/* Consumer - Producer No2 here */
                perror2("pthread_mutex_lock", err);		/* Lock mutex for condition var */
                exit(1);
        }

                if( err = pthread_mutex_lock(&list_mtx1)){	/* mutex for list */
                        perror2("pthread_mutex_unlock", err);
                        exit(1);
                }
		struct Encoded** level;
		string str = directory+"/"+FileName;
		level = TreeCreation(tree_height, fan_out, (char*)str.c_str());
		listH -> insert(level, FileName,-1);

                if( err = pthread_mutex_unlock(&list_mtx1)){	/* unlock sorted list so we can read from consumer the size */
                        perror2("pthread_mutex_unlock", err);
                        exit(1);
                }
	 /* Each time we finish a hash tree we send a signal to our Consumer that waits to finish all our trees */
		pthread_cond_signal( &cvar1);

        if( err = pthread_mutex_unlock(&cmtx1) ){	/* Unlock mutex for condition var */
                perror2("pthread_mutex_lock", err);
                exit(1);
        }

        if( err = pthread_mutex_lock(&term) ){		/* Terminating mutex */
                perror2("pthread_mutex_lock", err);
                exit(1);
        }
        Terminate = Terminate_WTH;
        if( err = pthread_mutex_unlock(&term) ){
                perror2("pthread_mutex_lock", err);
                exit(1);
        }

	}
        return NULL;

}


void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

/* -------------- */
/* COMPARE TREES  */
/* -------------- */
void compare_trees(struct Encoded**& server_tree,struct Encoded**& client_tree,string filename,int i,int j,int socket){
bool equal = true;
	int new_i=i;
	for(int k=0; k<4; k++)
		if( server_tree[new_i][j].digest[k] != client_tree[new_i][j].digest[k] )
			equal = false;

	if( equal == true ){
		if(new_i == 0)
			cout<<filename<<" is updated."<<endl;
		return;
	}
	else{
		new_i++;
		if( new_i < tree_height ){
			for(int z=j*fan_out; z<fan_out*(j+1); z++){
				compare_trees(server_tree,client_tree,filename,new_i,z,socket);}
		}
		else{
			cout<<"In File "<<filename<<" Problem at Page ["<<new_i<<"]["<<j<<"]"<<endl;
		        char *buffer;
        		buffer = new char[100];
			char page_num[10];
			sprintf(page_num," %d %d",new_i,j);
			string temp = "GETDATA "+directory+" "+filename+" "+page_num;
        		strcpy(buffer,temp.c_str());
			/* Sending GETDATA folder filename PageNo*/
			final = filename;		//hold the final filename in order to finish our client
        		send(socket, buffer ,100,0);
			return;
		}
	}

}

void* read_th(void* socket){
	int sock = (int)socket;
	int err;
	while(1){
		struct DATA_message msg;
		ssize_t recvd;
		recvd = recv(sock,&msg ,sizeof(struct DATA_message), MSG_WAITALL);
//		cout<<"GOT FIRST MESSAGE :D "<<endl;

		if( msg.die == 1){
			return NULL;}
		else{
			char* buff;
			buff = new char[PAGESIZE];
			recvd = recv(sock,buff ,PAGESIZE, MSG_WAITALL);
			FILE *fp;
			string path = directory+"/"+msg.filename;
			/* now lets compare the old filesize with the page size */
			struct stat st;
			stat(path.c_str(), &st);
			int size = st.st_size;
			if( size <= PAGESIZE )
				fp = fopen(path.c_str(), "w");
			else
				fp = fopen(path.c_str(), "r+");
			fseek(fp, msg.pagenum * PAGESIZE, SEEK_SET);
			fputs( buff,fp );
			fclose(fp);
			string x(msg.filename);
			//cout<<msg.filename<<endl;
			if( final.compare(x) == 0){	// in order to terminate
		                 /* Lock condition mutex */
//        		        if( err = pthread_mutex_unlock(&end_client) ){
//                		        perror2("pthread_mutex_lock", err);
  //                      		exit(1);
//                		}

			}

		}
	}

}

