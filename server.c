#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFFER_SIZE 256
char dir[BUFFER_SIZE];


void putStringinFile(char *filename, char *content) {
    FILE *fp = fopen(filename, "ab");
    if (fp != NULL)
    {
        fputs(content, fp);
        fclose(fp);
    }
}


char *stringFromFile(char *filename){
     char * buffer = NULL; 
     long length;
     FILE * f = fopen (filename, "rb");

     if (f){
         fseek (f, 0, SEEK_END);
         length = ftell (f);
         fseek (f, 0, SEEK_SET);
         buffer = malloc (length+1);
         if (buffer){
             fread (buffer, 1, length, f);
         }
         buffer[length]= '\0';

         fclose (f);
     }

     return buffer;
}


void *httpHandler(void *vargs) {
     int acceptid= *((int *)vargs);
     char msgok[]= "HTTP/1.1 200 OK\r\n\r\n";
     char msgnotfound[]= "HTTP/1.1 404 Not Found\r\n\r\n";
     
     char buffer[BUFFER_SIZE];
     read(acceptid, buffer, BUFFER_SIZE);
     char* type= strtok(buffer, " ");
     char* path= strtok(NULL, " ");
     char msg[BUFFER_SIZE];

     if(strcmp(type, "POST")==0){
         char filename[BUFFER_SIZE];
         char fullpath[BUFFER_SIZE];

         sscanf(path, "/files/%s", filename); 
         sprintf(fullpath, "%s/%s", dir, filename);

         int contentlength;

         strtok(NULL, "\r\n");
         strtok(NULL, "\r\n");
         strtok(NULL, "\n"); 
         strtok(NULL, "\n"); 
         char *fileContent= strtok(NULL, "\0");
         
         putStringinFile(fullpath, fileContent);
         sprintf(msg, "HTTP/1.1 201 Created\r\n\r\n");
     }
    
     else if (strncmp("/echo/", path, 6)==0){
         strcat(path, "/");
         char *echo= strtok(path, "/");
         char *string= strtok(NULL, "/");
         sprintf(msg, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s", strlen(string), string);
         char strsize[BUFFER_SIZE];
     }
     else if(strncmp("/user-agent", path, 11)==0){
         strtok(NULL, "\r\n");
         strtok(NULL, "\r\n");
         char *uahdr= strtok(NULL, "\r\n");
         char uastring[BUFFER_SIZE];
         sscanf(uahdr, "User-Agent: %s\r\n", uastring);
         sprintf(msg, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n%s", strlen(uastring), uastring);

     }
     else if(strncmp("/files/", path, 7)==0){
         char filename[BUFFER_SIZE];
         char fullpath[BUFFER_SIZE];

         sscanf(path, "/files/%s", filename); 
         sprintf(fullpath, "%s/%s", dir, filename);
         char *fileContent= stringFromFile(fullpath);
        
         if(fileContent != NULL){
            sprintf(msg, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n%s",\
                    strlen(fileContent), fileContent);
         }
         else if(fileContent == NULL){
             strncpy(msg, msgnotfound, BUFFER_SIZE);
         }

     }
     else if(strcmp(path, "/")==0){
         strncpy(msg, msgok, BUFFER_SIZE);
     }
     else {
         strncpy(msg, msgnotfound, BUFFER_SIZE);
     }

     send(acceptid, msg, strlen(msg), 0);
    
}



int main(int argc, char *argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

    if(argc==3 && strcmp("--directory", argv[1])==0) {
        strcpy(dir, argv[2]);
    }

	// Uncomment this block to pass the first stage
	//
	 int server_fd, client_addr_len;
	 struct sockaddr_in client_addr;
	
	 server_fd = socket(AF_INET, SOCK_STREAM, 0);
	 if (server_fd == -1) {
	 	printf("Socket creation failed: %s...\n", strerror(errno));
	 	return 1;
	 }
	
	 // Since the tester restarts your program quite often, setting REUSE_PORT
	 // ensures that we don't run into 'Address already in use' errors
	 int reuse = 1;
	 if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
	 	printf("SO_REUSEPORT failed: %s \n", strerror(errno));
	 	return 1;
	 }
	
	 struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
	 								 .sin_port = htons(4221),
	 								 .sin_addr = { htonl(INADDR_ANY) },
	 								};
	
	 if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	 	printf("Bind failed: %s \n", strerror(errno));
	 	return 1;
	 }
	
	 int connection_backlog = 5;
	 if (listen(server_fd, connection_backlog) != 0) {
	 	printf("Listen failed: %s \n", strerror(errno));
	 	return 1;
	 }
	
	 printf("Waiting for a client to connect...\n");
	 client_addr_len = sizeof(client_addr);
	 
     while(1){

	     int acceptid= accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
         if(acceptid!=-1)
	        printf("Client connected\n");
         else break;

         pthread_t newprocess;
         int *pacceptid= &acceptid;
         pthread_create(&newprocess, NULL, httpHandler, pacceptid);

     }
	 close(server_fd);

	 return 0;
}
