#include "blather.h"
#include <pthread.h>    /* required for pthreads */
#include <semaphore.h>  /* required for semaphores */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define getName(var) #var
client_t *server_get_client(server_t *server, int idx) {
// Gets a pointer to the client_t struct at the given index. If the
// index is beyond n_clients, the behavior of the function is
// unspecified and may cause a program crash.

  if(idx > server->n_clients){
    	printf("Invalid client number.\n");
	exit(1);
  } else {
	client_t actual_client = server->client[idx];
	client_t *client = &actual_client;
   return client; //expects pointer to a client
  }
}

void server_start(server_t *server, char *server_name, int perms){
// Initializes and starts the server with the given name. A join fifo
// called "server_name.fifo" should be created. Removes any existing
// file of that name prior to creation. Opens the FIFO and stores its
// file descriptor in join_fd.
//
  	strcpy(server->server_name, server_name);
	char name[MAXNAME];
	//char log_name[MAXNAME];
	//memset(name,0,MAXNAME);
	strcpy(name,server_name);
	strcat(name,".fifo");
	int exists = remove(name); 
 	if((exists==0)||(errno==ENOENT)){
 	 	mkfifo(name, perms);
		//printf("Join fifo %s created.\n", name);
  		server->join_fd = open(name,O_RDWR);
		server->join_ready = 0;
		//printf("Check: %d is the file descriptor to write to.\n", server->join_fd);
		//printf("Join fifo %s opened for reading and writing.\n", name);
	}
	else{
	perror("ERROR in server_start");
	printf("Call to function server_start() encountered an error. Aborting process.\n");
	abort();
	}



// ADVANCED: create the log file "server_name.log" and write the
// initial empty who_t contents to its beginning. Ensure that the
// log_fd is position for appending to the end of the file. Create the
// POSIX semaphore "/server_name.sem" and initialize it to 1 to
// control access to the who_t portion of the log.
 

		char log_name[MAXNAME];		
		strcpy(log_name,server_name);
		strcat(log_name,".log");
		server->log_fd = open(log_name, O_APPEND);// "a+"); // create and append to log
		if(server->log_fd == 0) {
			server_write_who(server);
		}
		server->log_fd = open(log_name, O_APPEND);
		char sem_name[MAXNAME];		

		strcpy(sem_name,server_name);
		snprintf(sem_name + strlen(sem_name), MAXNAME - strlen(sem_name), ".sem");
		sem_t *pSemaphore = sem_open(sem_name,O_CREAT);
		if(pSemaphore == SEM_FAILED){
			perror("Error in server_start");
			abort();
		}
		sem_init(pSemaphore, 0, 1);

		server->log_sem = pSemaphore;

		

}

void server_shutdown(server_t *server){
// Shut down the server. Close the join FIFO and unlink (remove) it so
// that no further clients can join. Send a BL_SHUTDOWN message to all
// clients and proceed to remove all clients in any order.
//
	mesg_t message;
	mesg_t *mesg = &message;
	mesg->kind=BL_SHUTDOWN;
	server_broadcast(server,mesg);
	
	//sleep(3);

	for(int i=0; i < server->n_clients; i++){
		server_remove_client(server, i);
	}
	char semop_name[MAXNAME];
	strcpy(semop_name, server->server_name);
	int close_join = close(server->join_fd);
	int remove_server = remove(strcat(server->server_name,".fifo"));
	int close_log = close(server->log_fd);
	int close_semop = sem_close(server->log_sem);
	int unlink_semop = sem_unlink(strcat(semop_name,".sem"));
	
	int error_check[5]={close_join, remove_server, close_log, close_semop, unlink_semop};
	for(int i=0; i < 2; i++){
		//printf("%s value: %d\n", getName(error_check[i]), error_check[i]);
		if(error_check[i]==-1){
			perror("Error in server shutdown");
			abort();
			}
		}
return;
}

int server_add_client(server_t *server, join_t *join){
// Adds a client to the server according to the parameter join which
// should have fileds such as name filed in.  The client data is
// copied into the client[] array and file descriptors are opened for
// its to-server and to-client FIFOs. Initializes the data_ready field
// for the client to 0. Returns 0 on success and non-zero if the
// server as no space for clients (n_clients == MAXCLIENTS).

  if(server->n_clients == MAXCLIENTS){
    return 1;
	  } 
else {
		server->n_clients++; //incremented at top of the else just for niceness
	
		//initialize new client struct
     		client_t actual_client = server->client[server->n_clients];
		client_t *client = &actual_client;

		strcpy(client->name,join->name);
		//get file descriptors
		client->to_client_fd = open(join->to_client_fname, O_RDWR);
		client->to_server_fd = open(join->to_server_fname, O_RDWR);
		//get file names/paths
		strcpy(client->to_client_fname,join->to_client_fname);
		strcpy(client->to_server_fname,join->to_server_fname);
		//time and data ints
		client->data_ready = 0;
		client->last_contact_time = server->time_sec;

      		return 0;
	  }

}

int server_remove_client(server_t *server, int idx){
// Remove the given client likely due to its having departed or
// disconnected. Close fifos associated with the client aerver_add_client(server_t *server, join_t *join);nd remove
// them.  Shift the remaining clients to lower indices of the client[]
// array and decrease n_clients.

//printf("Removing client. Before remove: %d\n", server->n_clients);
//printf("Client being removed:\nName: %s\n", server->client[idx].name);
if(idx == server->n_clients){
	server->n_clients--;
	}
else if(idx < server->n_clients){
	for(int i=idx; i < server->n_clients; i++){
		server->client[i] = server->client[i+1];
	}
	server->n_clients--;
	}
else if(idx > server->n_clients){
	printf("Can't remove client %d; outside of bounds.\n", idx);
}
//printf("After removal: %d\n", server->n_clients);
//printf("Remaining client: %s\n", server->client[0].name);
return 0;

}

int server_broadcast(server_t *server, mesg_t *mesg){
// Send the given message to all clients connected to the server by
// writing it to the file descriptors associated with them.
//
  int i;
  int size;
	size = sizeof(mesg_t);
	mesg_t message;
	mesg_t *mesg_p=&message;
	memset(mesg_p,0,size);
	mesg_p = mesg;
	for(i=0; i< server->n_clients; i++){  
		write(server->client[i].to_client_fd, mesg_p, size);	
	}

// ADVANCED: Log the broadcast message unless it is a PING which
// should not be written to the log.

/*

	if(mesg_p->kind == BL_PING){
		//Do nothing 
	}
	else {
		write(server->log_name, mesg_p, size);
	}

*/

return 0;
}

void server_check_sources(server_t *server){
// Checks all sources of data for the server to determine if any are
// ready for reading. Sets the servers join_ready flag and the
// data_ready flags of each of client if data is ready for them.
// Makes use of the select() system call to efficiently determine
// which sources are ready.
// TWO CASES: join request, or message sending.
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(server->join_fd, &fdset);
	for(int i=0; i < server->n_clients; i++){
		FD_SET(server->client[i].to_server_fd, &fdset);
	}

	select(MAXCLIENTS+1, &fdset, NULL, NULL, NULL);
      	if(FD_ISSET(server->join_fd, &fdset)){
		server->join_ready = 1;
  		}
	for(int i=0; i < server->n_clients; i++){
		if(FD_ISSET(server->client[i].to_server_fd, &fdset)){
			server->client[i].data_ready=1;
			}
		}
}

int server_join_ready(server_t *server){
// Return the join_ready flag from the server which indicates whether
// a call to server_handle_join() is safe.
	return server->join_ready;
}

int server_handle_join(server_t *server){
// Call this function only if server_join_ready() returns true. Read a
// join request and add the new client to the server. After finishing,
// set the servers join_ready flag to 0.
	if(server_join_ready(server)){
		join_t join;
		memset(&join,0,sizeof(join_t));
		//printf("%d",read(server->join_fd, &join, sizeof(join)));
		//printf("Got to server handle join.\n");
		/*int result = */read(server->join_fd, &join, sizeof(join));
		//while(result!=0){
		//	if(errno!=EINTR){
		//		perror("Error in server_handle_join");
		//		abort();
		//	}
		//	result = read(server->join_fd, &join, sizeof(join));
			//printf("%d", result);	
		//}
		//printf("Finished reading join.\n");
		strcpy(server->client[server->n_clients].name,join.name);
		//printf("Opening to client fd...\n");
		server->client[server->n_clients].to_client_fd = open(join.to_client_fname, O_RDWR);
		//printf("Opening to server fd...\n");
		server->client[server->n_clients].to_server_fd = open(join.to_server_fname, O_RDWR);
		server->client[server->n_clients].data_ready = 0;
		server->client[server->n_clients].last_contact_time = server->time_sec;
		mesg_t actual_mesg;
		mesg_t *join_mesg = &actual_mesg;
		memset(join_mesg,0,sizeof(mesg_t));
		strcpy(join_mesg->name, join.name);
		join_mesg->kind = BL_JOINED;
		server->n_clients++;
		//printf("In server handle join: %d clients.\n", server->n_clients);
		server_broadcast(server, join_mesg);
 		server->join_ready = 0;
		//printf("Finished server handle join.\n");
		return 0;
	}
	else{
		printf("Server_join_ready() is false.\n");
		abort();
	}
}

int server_client_ready(server_t *server, int idx){
// Return the data_ready field of the given client which indicates
// whether the client has data ready to be read from it.
client_t client = server->client[idx];
int result = client.data_ready;
return result;
}

int server_handle_client(server_t *server, int idx){
// Process a message kind. Departure
// and Message types should be broadcast to all other clients.  Ping
// responses should only change the last_contact_time below. Behavior
// for other message types is not specified. Clear the client's
// data_ready flag so it has value 0.
//
// ADVANCED: Update the last_contact_time of the client to the current
// server time_sec.
	mesg_t mesg;
	mesg_t *mesg_p = &mesg;
	//memset(mesg_p,0,sizeof(mesg_t)); //turned off at 8:51pm
  	if(server_client_ready(server,idx)){
    		read(server->client[idx].to_server_fd, mesg_p, sizeof(mesg_t));
		if(mesg_p->kind == BL_DEPARTED){
			server_remove_client(server,idx);
		}
		//if(mesg_p->kind!=BL_PING){
			server_broadcast(server, mesg_p);
    			server->client[idx].data_ready = 0;
    			server->client[idx].last_contact_time = server->time_sec;
		//	}
		//else{
		//	printf("Recieved ping from %s\n", server->client[idx].name);
		//	server->client[idx].data_ready = 0;
    		//	server->client[idx].last_contact_time = server->time_sec;
		//}
  	}
	return 0;
}

void server_tick(server_t *server){
// ADVANCED: Increment the time for the server
  server->time_sec++;
}

void server_ping_clients(server_t *server){
// ADVANCED: Ping all clients in the server by broadcasting a ping.
	printf("check 0\n");
	for(int i=0; i < server->n_clients; i++){
		printf("%s: check 1\n",server->client[i].name);
		mesg_t message;
		mesg_t *mesgp = &message;
		memset(mesgp,0,sizeof(mesg_t));
		mesgp->kind = BL_PING;
		strcpy(mesgp->name, server->client[i].name);
		printf("%s: check 2\n",server->client[i].name);
		if(!write(server->client[i].to_client_fd, mesgp, sizeof(mesg_t))){
			perror("Error in server_ping_clients");
			abort();
		}
		printf("%s: check 3\n",server->client[i].name);
	}
	
	return;
}

void server_remove_disconnected(server_t *server, int disconnect_secs){
// ADVANCED: Check all clients to see if they have contacted the
// server recently. Any client with a last_contact_time field equal to
// or greater than the parameter disconnect_secs should be
// removed. Broadcast that the client was disconnected to remaining
// clients.  Process clients from lowest to highest and take care of
// loop indexing as clients may be removed during the loop
// necessitating index adjustments.
printf("Removing disconnected users.\n");

for(int i = 0; i < server->n_clients; i++){
	int elapsed = server->time_sec - server->client[i].last_contact_time;
	printf("ELAPSED TIME %d\n", elapsed);
  	if(elapsed >= disconnect_secs){
		printf("Removing client %s\n", server->client[i].name);
		mesg_t disconnect_message;
		mesg_t *disconnect = &disconnect_message;
	
		strcpy(disconnect->name, server->client[i].name);
		disconnect->kind = BL_DISCONNECTED;
		server_broadcast(server, disconnect);

		server_remove_client(server, i);
  		}
	}
printf("Number of clients now: %d\n", server->n_clients);
}

void server_write_who(server_t *server){
// ADVANCED: Write the current set of clients logged into the server
// to the BEGINNING the log_fd. Ensure that the write is protected by
// locking the semaphore associated with the log file. Since it may
// take some time to complete this operation (acquire semaphore then
// write) it should likely be done in its own thread to preven the
// main server operations from stalling.  For threaded I/O, consider
// using the pwrite() function to write to a specific location in an
// open file descriptor which will not alter the position of log_fd so
// that appends continue to write to the end of the file.

/**

  who_t *who;
  who->n_clients = server->n_clients;
  char message[MAXLINE];

  int i, j;
  for(i=0;i<who->n_clients;i++){
    who->names[i][0]=server->client[i].name;
  }

  strcpy(message,"====================\n");
  strcat(message, who->n_clients + "CLIENTS\n");
    for(j=0; j<who->n_clients; j++){
      strcat(message, ("%d: %s\n", j, who->names[j][0])); 
    }  
  strcat(message, "====================\n");

    pwrite(server->log_fd, message, sizeof(message), 0);
**/
/**
  if(sem_wait(pSemaphore)){
    pwrite(server->log_fd, message, sizeof(message), 0);
  }

 

**/

}

void server_log_message(server_t *server, mesg_t *mesg){
// ADVANCED: Write the given message to the end of log file associated
// with the server.

/**
int i, log;
log = fopen (server->log_fd,"a");// use "a" for append
for(i=0;i<sizeof(mesg->body);i++){
 fprintf (log, "%c", mesg->body[i] );// character array to file
}


**/
}
