//127.0.0.2:1998
#include "ftp_message.h" 
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>

char    *root = "/home/ted/Desktop/";

void*   communication(void *client);
void    handle_client_command(int handle_cmd_sock);
void    recv_msg(int socket, char** buf, char** cmd, char** argument);
int     establish_tcp_connection(int *client_data_sock, char *client_ip, int *server_data_port, int *server_data_sock);
void    cancel_tcp_connection(int *data_sock, char *client_ip, int *server_data_port);

void handle_USER(int handle_cmd_sock, char *argument);
void handle_AUTH(int handle_cmd_sock, char *argument);
void handle_USER(int handle_cmd_sock, char *argument);
void handle_PWD(int handle_cmd_sock, char *working_dir);
void handle_CWD(int handle_cmd_sock, char *working_dir,char *argument);
void handle_SYST(int handle_cmd_sock);
void handle_TYPE(int handle_cmd_sock, char *argument);
void handle_PASV(int handle_cmd_sock, int *data_sock, char *client_ip, int *server_data_port);
void handle_PORT(int handle_cmd_sock, int *data_sock, char *client_ip, int *server_data_port, char *argument);
void handle_LIST(int handle_cmd_sock, int *data_sock, char *client_ip, int *server_data_port, char *working_dir, int *server_data_sock);

int main(int clientc, char *clientv[])
{
    /*Init Server*/
    struct sockaddr_in sAddr;           // Address of server
    int listensock;                     // A socket to listen to client connections on
    int client;                         // A socket to handle client after its connection
    int number_client;
    int result;
    pthread_t thread_id;
    int val;

    listensock = socket(AF_INET, SOCK_STREAM, 0);

    val = 1;
    result = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (result < 0) {
        perror("server4");
        return 0;
    }


    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons(1998);
    sAddr.sin_addr.s_addr = inet_addr("127.0.0.2");
    result = bind(listensock, (struct sockaddr *) &sAddr, sizeof(sAddr));
    if (result < 0) {
        perror("server4");
        return 0;
    }

    /*Server starts*/
    result = listen(listensock, 5);
    if (result < 0) {
        perror("server4");
        return 0;
    }
    printf("Server is listening on port 1998 now.\n");
    while (1) {
        number_client = 0;
        client = accept(listensock, NULL,NULL);
        number_client ++;
        printf("Server now handle client %d on a new socket (%d).\n", number_client, client);
        result = pthread_create(&thread_id, NULL, communication, (void *) client);
        if (result != 0) {
            printf("Could not create thread.\n");
            number_client --;
        }
        /*Now because the parent thread will be in continuos loop
         and dont need to ever join one of child threads so we call 
         pthread_detach() to keep zombies from occurring.

         A zombie is a process or thread that has returned and is 
         waiting for its parent check its value, so it takes up
         resources until its value is checked

         The we call sched_yield() to give up the remainder of the
         parent allotted time slice. A new thread will have a chance
         to execute.
        */
        pthread_detach(thread_id);
        sched_yield();
    }
}

void* communication(void *client)
{
    int client_sock;
    char buffer[1000];
    int nread;
    printf("Client's thread %i with pid %i created.\n", pthread_self(), getpid());
    client_sock = (int) client;
   
    //Send welcome message
    sprintf(buffer, "220 Welcome to my FPT server.\r\n");
    write(client_sock, buffer, strlen(buffer));
    printf("Server: %s\n", buffer);

    handle_client_command(client_sock);

    //close(client_sock);
    //printf("Client's thread %i with pid %i finished.\n", pthread_self(),getpid());
}

//Handle clients
void handle_client_command(int handle_cmd_sock) {
    char    *buffer = (char*) malloc(sizeof(char) * 1000);

    /*Informations for each session of work*/
    int     server_connection_socket = handle_cmd_sock;         //Socket is used for FTP commands transferring
    int     server_data_socket = 0;                             //Socket is used for data transferring, init value = 0 (not in used)
    int     client_data_socket = 0;
    int     server_data_port;                                   //Port is used for data transferring
    char    client_ip[20];                                      //Client's address
    char    *command = NULL;
    char    *argument = NULL;
    char    working_dir[] = "/home/ted/Desktop/";                //Init working directory is root

    while(1)
    {
        /*Receive command*/
        recv_msg(handle_cmd_sock, &buffer, &command, &argument);
        
        if (strcmp("AUTH", command) == 0){
            handle_AUTH(handle_cmd_sock, argument);
        }        
        else if (strcmp("USER", command) == 0){
            handle_USER(handle_cmd_sock, argument);
        }        
        else if (strcmp("PWD", command) == 0){
            handle_PWD(handle_cmd_sock, working_dir);
        }
        else if(strcmp("TYPE", command) == 0){
            handle_TYPE(handle_cmd_sock, argument);
        }
        else if(strcmp("PASV", command) == 0){
            handle_PASV(handle_cmd_sock, &server_data_socket , client_ip, &server_data_port);
            /*
            char buffer[1000];
            sprintf(buffer, "502 Command not implemented.\r\n");
            write(handle_cmd_sock, buffer, strlen(buffer));
            printf("Server: %s\n\n", buffer);
            */
        }
        else if (strcmp("SYST", command) == 0){
            handle_SYST(handle_cmd_sock);
        }     
        else if (strcmp("PORT", command) == 0){
            handle_PORT(handle_cmd_sock, &server_data_socket , client_ip, &server_data_port, argument);
        }
        else if(strcmp("LIST", command) == 0){
            handle_LIST(handle_cmd_sock, &client_data_socket, client_ip, &server_data_port, working_dir, &server_data_socket);
        }
        else if (strcmp("CWD", command) == 0) {
			handle_CWD(handle_cmd_sock, working_dir, argument);
        }
        else
        {
            char buf[100];
			strcpy(buf, "500 ");
			strcat(buf, command);
			strcat(buf, " cannot be recognized by server\r\n");
			write(handle_cmd_sock, buf, strlen(buf));
            printf("Server: %s\n\n", buf);
        }
    }   

}
//Receive message from client
void recv_msg(int socket, char** buf, char** cmd, char** argument) {
    int nread;
    memset(*buf, 0, sizeof(char) * 1000);

    nread = recv(socket, *buf, 1000, 0);
    
    if (nread == 0) {
		printf("Client leave the server.\n");
		pthread_exit(NULL );
	}

    int index = _find_first_of(*buf, ' ');
	if (index < 0) {
		*cmd = _substring(*buf, 0, strlen(*buf) - 2);

	} else {
		*cmd = _substring(*buf, 0, index);
		*argument = _substring(*buf, index + 1, strlen(*buf) - index - 3);

	}
	if (nread < 0) {
		perror("recv msg error...");
	} else {
        printf("Client: %s\n", *buf);
	}
}

/*This function will handle TCP connection in two case:
    - Active case (PORT command)
    - Passive case (PASV command)
 The value returned:
    1 : success
    -1: failed
*/
int establish_tcp_connection(int *client_data_sock, char *client_ip, int *server_data_port, int *server_data_socket){
    // If client_ip is not NULL --> Active case
    if (*client_ip){
        *client_data_sock = socket(AF_INET, SOCK_STREAM, 0);
        printf("Server: data_sock = %d\n\n", *client_data_sock);
        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(*server_data_port);    //In active mode, server data port is usually the same as client's
        printf("Server: data host port %d\n\n", ntohs(client_addr.sin_port));
        if ( inet_pton(AF_INET ,client_ip, &client_addr.sin_addr) <= 0){
            printf("Server: Error in PORT command.\n\n");
            return -1;
        }
        printf("Server: client address %s\n\n", client_ip);
        if ( connect(*client_data_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) == 0) {
            printf("Server: PORT connect successfully.\n\n");
        }
        else{
            printf("Server: Failed to connect to client in active mode.\n\n");
            return -1;
        }
        
    }
    // If client_ip  is NULL but data_sock > 0  --> Passive case
    else if( *server_data_socket > 0 ){
        socklen_t sock = sizeof(struct sockaddr);
		struct sockaddr_in data_client;
		*client_data_sock = accept(*server_data_socket,(struct sockaddr*) &data_client, &sock);

		if (client_data_sock < 0) {
			printf("Server:[passive] accept error\n\n");
            return ;
		} 
        else {
			socklen_t sock_length = sizeof(struct sockaddr);
			char client_ip[100];
			getpeername(*client_data_sock, (struct sockaddr*) &data_client, &sock_length);
			inet_ntop(AF_INET, &(data_client.sin_addr), client_ip, INET_ADDRSTRLEN);
			printf("Server:[passive] %s connect to the host.\n\n", client_ip);
		}
    }

    return 1;
}

void cancel_tcp_connection(int *data_sock, char *client_ip, int *server_data_port){
    if (*client_ip){
        *client_ip = 0;
        *server_data_port = 0;
    }
    if (*data_sock > 0){
        close(*data_sock);
        *data_sock = -1;
    }
    printf("Server: Cancelled data transfering.\n\n");
}

void handle_AUTH(int handle_cmd_sock, char *argument){
    char buffer[1000];
    //Send response AUTH TLS/SSL message
    sprintf(buffer, "530 Please login with USER and PASS.\r\n");
    write(handle_cmd_sock, buffer, strlen(buffer));
    printf("Server: %s\n\n", buffer);
}
void handle_USER(int handle_cmd_sock, char *argument){
    char buffer[1000];
    if (strcmp("anonymous", argument) == 0 ){
        //Send login successful message
        sprintf(buffer, "230 Login successful.\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
    }
    else
    {
        sprintf(buffer, "230 Login successful.\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        //We will add login mechanism later
    }    
    printf("Server: %s\n\n", buffer);
}
void handle_PWD(int handle_cmd_sock, char *working_dir){
    char buffer[1000];
    strcpy(buffer, "257 \"");
    strcat(buffer, working_dir);
    strcat(buffer, "\"\r\n");
    write(handle_cmd_sock, buffer, strlen(buffer));
    printf("Server: %s\n\n", buffer);
}
void handle_SYST(int handle_cmd_sock){
    char buffer[1000];
    sprintf(buffer, "215 UNIX Type: L8\r\n");
    write(handle_cmd_sock, buffer, strlen(buffer));
    printf("Server: %s\n\n", buffer);
}
void handle_TYPE(int handle_cmd_sock, char *argument) {
    
	char type[] = "200 Switching to Binary mode.\r\n";
	write(handle_cmd_sock, type, strlen(type));
    printf("Server: %s\n\n", type);
    
}

/*This function will:
    - Clear data socket if it exist
    - Create a socket to listen to client's data connections
*/
void handle_PASV(int handle_cmd_sock, int *data_sock, char *client_ip, int *server_data_port){
    if (*data_sock > 0){
        close(*data_sock);
        *data_sock = -1;
    }
    *data_sock = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[1000];

    if (*data_sock < 0){
        sprintf(buffer, "496 PASV failure\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server:[create socket] %s\n\n", buffer);
        return ;
    }
    struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1"); //We need to change "127.0.0.1" to client_ip later.
	server.sin_port = htons(0);

    if (bind(*data_sock, (struct sockaddr*) &server,sizeof(struct sockaddr)) < 0) {
		sprintf(buffer, "496 PASV failure\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server:[bind socket] %s\n\n", buffer);
        return ;
	}

    printf("Server: Server is established, waiting for data connect ...\n\n");

    if (listen(*data_sock, 1) < 0) {
		sprintf(buffer, "496 PASV failure\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server:[listen] %s\n\n", buffer);
        return ;
	}

    struct sockaddr_in file_addr;
	socklen_t file_sock_len = sizeof(struct sockaddr);
	getsockname(*data_sock, (struct sockaddr*) &file_addr,
			&file_sock_len);

	int port = ntohs(file_addr.sin_port);
	char* msg = _transfer_ip_port_str("127.0.0.1", port);
	char buffer2[200];
	strcpy(buffer2, "227 Entering Passive Mode (");
	strcat(buffer2, msg);
	strcat(buffer2, ")\r\n");
	write(handle_cmd_sock, buffer2, strlen(buffer2));
    printf("Server: %s\n\n", buffer2);
    *server_data_port = port;
	free(msg);

}
void handle_PORT(int handle_cmd_sock, int *data_sock, char *client_ip, int *server_data_port, char *argument){
    //If data sock is used, close it!
    if (*data_sock > 0){
        close(*data_sock);
    }
    *client_ip = 0;             //NULL, client's ip have been not saved here yet.
    int h1, h2, h3, h4, p1, p2;
    sscanf(argument, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    sprintf(client_ip, "%d.%d.%d.%d", h1, h2, h3, h4);

    *server_data_port = p1*256 + p2;

    char buffer[1000];
    sprintf(buffer, "200 PORT command successful\r\n");
    write(handle_cmd_sock, buffer, strlen(buffer));

    printf("Server: 200 PORT command successful | Client address: %s | Data host port: %d\n\n", client_ip, *(server_data_port));

}
void handle_LIST(int handle_cmd_sock, int *data_sock, char *client_ip, int *server_data_port, char *working_dir, int *server_data_socket){
    FILE *pipe_fp = NULL;
    char list_cmd_info[200];
	char path[200];
    char buffer[1000];
    sprintf(path, "%s" , working_dir);
    sprintf(list_cmd_info, "ls -l %s", path);

    if ((pipe_fp = popen(list_cmd_info, "r")) == NULL){
        sprintf(buffer, "451 The server had trouble reading the directory from disk\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server: %s\n\n", buffer);
        return ;
    }

    if (establish_tcp_connection(data_sock, client_ip, server_data_port, server_data_socket) == 1){
        sprintf(buffer, "150 Data connection accepted, transfer starting \r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server: %s\n\n", buffer);

        char buffer2[10000];
        fread(buffer2, 10000 - 1, 1, pipe_fp);
        int l = _find_first_of(buffer2, '\n');
        
        write(*data_sock, &buffer2[l+1], strlen(&buffer2[l+1]));
        printf("Data  :\n%s\n\n", &buffer2[l+1]);
        pclose(pipe_fp);
        cancel_tcp_connection(data_sock, client_ip, server_data_port);
        

        sprintf(buffer, "226 Transfer OK\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server: %s\n\n", buffer);


    }
    else{
        sprintf(buffer, "425 TCP connection cannot be established \r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        cancel_tcp_connection(data_sock, client_ip, server_data_port);
        printf("Server: %s\n\n", buffer);
        return ;
    }
}
void handle_CWD(int handle_cmd_sock, char *working_dir, char *argument){
    int flag = 0;
	if (argument[0] != '/') {
		flag = 1;
	}
    char buffer[1000];
    strcpy(buffer, argument);
    strcat(buffer, "/");

    DIR * openable = NULL;
	openable = opendir(buffer);
    //If directory exist
    if ( openable != NULL ) {		
		sprintf(working_dir,"%s", buffer); 
        printf("Server: working directry %s\n\n", working_dir);
        sprintf(buffer, "250 Okay\r\n");
        write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server: %s\n\n", buffer);

	} 
    //If directory does not exist
    else {
        sprintf(buffer,"550 No such file or directory.\r\n"); 
		write(handle_cmd_sock, buffer, strlen(buffer));
        printf("Server: %s\n\n", buffer);
	}    
    closedir(openable);
}


