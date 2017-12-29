#include "blather.h"

int ext_check(char *match, char *pattern);
char *read_all(int fd, int *nread);

int main(int argc, char **argv){
	//validity checking	
	if(argc != 2){
		printf("Invalid number of arguments: %d\n",argc);
		exit(1);
		}
	else if(ext_check(argv[1],".log")!=0){
		printf("File entered is not a .log file.\n");
		exit(1);
	}
	//open file for reading	
	int fd = open(argv[1],O_CREAT, DEFAULT_PERMS);
	if(fd == -1){
		perror("Error opening file");
		exit(1);
	}
	who_t actual_who;
	who_t *who = &actual_who;
	read(fd, who, sizeof(who));
	for(int i=0; i < who->n_clients; i++){
		printf("%d: %s\n",i, who->names[i]);
	}
	int nbytes = -1;
	void *output = NULL;
	int output_size;
	output = read_all(fd, &nbytes);
	output_size = nbytes;
	printf("- - - START LOG - - -\n");
	write(STDERR_FILENO,output,output_size);
	printf("\n- - - END LOG - - -\n");
	close(fd);
	free(output);

}

int ext_check(char *match, char *pattern){
	//heavily altered kmp algorithm lol
	int e = strlen(pattern)-1;
	int m = strlen(match)-1;
	int is_log = 0;
	while(is_log){
		if(pattern[e] == match[m]){
			e--;
			m--;
			if(e == -1){
				break;
			}
		}else{
			is_log =1;
			break;
		}
	}
	return is_log;
		
}
//read_all function from xiem's commando
char *read_all(int fd, int *nread){
	char *buf = malloc(MAXNAME);
	int bufsize = MAXNAME;
	int total_read = 0;
        int nbytes;
	while( ((nbytes = read(fd, buf+total_read, (bufsize- total_read))) > 0)){
		bufsize = bufsize *2;		
		buf = realloc(buf, bufsize);
		total_read += nbytes;
		if(nbytes == -1){
			perror("Error");
			exit(1);
		}
	}
	buf[total_read+1] = '\0';
	*nread = total_read; //since nread will be 0 upon exiting the loop, reset it
	return buf;
}
