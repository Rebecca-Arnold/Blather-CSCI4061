
//bl-client.c
/**
read name of server and name of user from command line args
create to-server and to-client FIFOs
write a join_t request to the server FIFO
start a user thread to read inpu
start a server thread to listen to the server
wait for threads to return
restore standard terminal output
user thread{
  repeat:
    read input using simpio
    when a line is ready
    create a mesg_t with the line and write it to the to-server FIFO
  until end of input
  write a DEPARTED mesg_t into to-server
  cancel the server thread
server thread{
  repeat:
    read a mesg_t from to-client FIFO
    print appropriate response to terminal with simpio
  until a SHUTDOWN mesg_t is read
  cancel the user thread
**/

#include "blather.h"
#include <time.h>
#include <dirent.h>

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

server_t server_actual;
server_t *server = &server_actual;

pthread_t user_thread;
pthread_t server_thread;

void *user_worker(){
	while(!simpio->end_of_input){
		simpio_reset(simpio);
		iprintf(simpio,"");
		while(!simpio->line_ready && !simpio->end_of_input){
			simpio_get_char(simpio);
		}
		if(simpio->end_of_input){
			mesg_t message;
			mesg_t *mesg_p = &message;
			memset(mesg_p, 0, sizeof(mesg_t));
			mesg_p->kind = BL_DEPARTED;
			strcpy(mesg_p->name, client->name);
			write(client->to_server_fd, mesg_p, sizeof(mesg_t));
		}
		if(simpio->line_ready){
			mesg_t message;
			mesg_t *mesg_p = &message;
			memset(mesg_p, 0, sizeof(mesg_t));
			mesg_p->kind = BL_MESG;
			strcpy(mesg_p->name, client->name);
			strcpy(mesg_p->body, simpio->buf);
			write(client->to_server_fd, mesg_p, sizeof(mesg_t));
		}
	}

	pthread_cancel(server_thread);
	return NULL;
}

void *server_worker(){
int shutdown = 0;
while(shutdown==0){
	mesg_t message;
	mesg_t *mesg_p = &message;
	read(client->to_client_fd, mesg_p, sizeof(mesg_t));
	time_t t;
			struct tm now;
			t=time(&t);
			localtime_r(&t, &now);
	switch(mesg_p->kind){
		case BL_JOINED:
			printf("%s", mesg_p->name);
			iprintf(simpio, "-- %s JOINED --\n", mesg_p->name);
			break;
		case BL_DEPARTED:
			iprintf(simpio, "-- %s DEPARTED --\n", mesg_p->name);
			break;
		case BL_MESG:
			iprintf(simpio, "[%s] : %s\n", mesg_p->name, mesg_p->body);
//			iprintf(simpio, "%s[%02d:%02d]>> %s\n",mesg_p->name, now.tm_hour, now.tm_min, mesg_p->body);
			break;
		//case BL_PING:
		//	write(client->to_server_fd, mesg_p, sizeof(mesg_t));
		//	iprintf(simpio,"%s pinged back\n", mesg_p->name);
		//	break;
		case BL_SHUTDOWN:
			shutdown=1;
		default:
			break;
			}
}
iprintf(simpio, "!!! server is shutting down !!!\n");
//for(int i=3; i > -1; i--){
//	iprintf(simpio, "[SERVER: %d second(s) remaining.]\n", i);
//	sleep(1);
//	}
pthread_cancel(user_thread);
return NULL;
}

int main(int argc, char **argv){
	setvbuf(stdin, NULL, _IONBF,0);
	//memset(server,0,sizeof(server_t));
	//memset(client,0,sizeof(client_t));
	if(sizeof(argv)/4 != 2){
		printf("%lu Invalid number of arguments.\n", sizeof(argv)/sizeof(char));
		exit(1);
	}
	strcpy(server->server_name,argv[1]);
	char server_fifo[MAXNAME];
	strcpy(server_fifo,server->server_name);
	strcat(server_fifo,".fifo");
	char user[MAXNAME];
	strcpy(user,argv[2]);
	strcpy(client->name,user);
	char prompt[MAXNAME];
	DIR *f;
	struct dirent *folder;
	f = opendir(".");
	if(f){
		int found = 0;
		while((folder = readdir(f))!=NULL){
			if(strcmp(folder->d_name,server_fifo)==0){
				found = 1;
				break;
			}
			}
		if(found == 0){
			printf("%s doesn't exist.\n", server_fifo );
			closedir(f);
			abort();		
		}
		closedir(f);	
	}

	join_t join;
	memset(&join,0,sizeof(join_t));
	strcpy(join.name,user);
	char client_fname[MAXPATH];
	char server_fname[MAXPATH];
	snprintf(client_fname,sizeof(long),"%ld",(long)getpid());
	snprintf(server_fname,sizeof(long),"%ld",(long)getpid());
	strcpy(join.to_client_fname, strcat(client_fname,".client.fifo"));
	strcpy(join.to_server_fname, strcat(server_fname,".server.fifo"));
	mkfifo(join.to_client_fname, S_IWUSR | S_IRUSR);
	mkfifo(join.to_server_fname, S_IWUSR | S_IRUSR);
	
	client->to_client_fd=open(join.to_client_fname, O_RDWR);
	client->to_server_fd=open(join.to_server_fname, O_RDWR);

	server->join_fd = open(server_fifo, O_RDWR);
	write(server->join_fd, &join, sizeof(join));
	
	snprintf(prompt, MAXNAME, "%s>> ", user); //create prompt with 1TH argument
	simpio_set_prompt(simpio, prompt);
	simpio_reset(simpio);
	simpio_noncanonical_terminal_mode();
	pthread_create(&user_thread, NULL, user_worker, NULL); //start user thread
	pthread_create(&server_thread, NULL, server_worker, NULL); //start server thread
	pthread_join(user_thread, NULL);
	pthread_join(server_thread, NULL);
	simpio_reset_terminal_mode();
	printf("\n");
	return 0;
}
