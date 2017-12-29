//bl-server.c
/**
REPEAT:
  check all sources
  handle a join request if on is ready
  for each client{
    if the client is ready handle data from it
  }
}
**/
#include "blather.h"
#include <stdbool.h>

server_t actual_server;
server_t *server = &actual_server;

//signal handling so that CTRL-C will signal the server
int signaled = 0;
int disconnected = 0;

void handle_shutdown(int signo){
	if(signo == SIGTERM || signo == SIGINT){ 
	char *msg = "Server signaled to shutdown. Calling server_shutdown().\n";
	write(STDERR_FILENO,msg,strlen(msg));
	//sleep(3);
	server_shutdown(server);
	signaled = 1; 
	exit(0);                     
//	return;
	}
}

/*void handle_alarm(int signo){
	if(signo == SIGALRM){
	char *msg = "handling alarm?\n";
	write(STDERR_FILENO, msg, strlen(msg));
	disconnected = true;
	alarm(1);
	}
}*/

int main(int argc, char *argv[]){
	setvbuf(stdin, NULL, _IONBF, 0);

	struct sigaction shutdown = {};
 	shutdown.sa_handler = handle_shutdown;
  	sigemptyset(&shutdown.sa_mask);
  	shutdown.sa_flags = SA_RESTART;
  	sigaction(SIGTERM, &shutdown, NULL);
 	sigaction(SIGINT,  &shutdown, NULL);

	/*struct sigaction ping = {};
	ping.sa_handler = handle_alarm;
	sigemptyset(&ping.sa_mask);
	ping.sa_flags = SA_NODEFER;
	sigaction(SIGALRM, &ping, NULL);
	alarm(1);*/
	//memset(server,0,sizeof(server_t));
	server_start(server,argv[1],DEFAULT_PERMS);
	//printf("Opened fifos. Listening...Press CTRL-C to shutdown.\n");
	while(signaled == 0){

		server_check_sources(server);
		if(server_join_ready(server)){
			server_handle_join(server);
		}
		for(int i=0; i < server->n_clients; i++){
			if(server_client_ready(server, i)){
				//printf("Got to server handle client\n");
				server_handle_client(server, i);		
			}	
		}
		/*if(disconnected){
			printf("Got into disconnected if\n");
			server_ping_clients(server);
			printf("Pinged clients\n");
			server_remove_disconnected(server, 5);
			disconnected = false;
		}*/
	}
	exit(0);
}
