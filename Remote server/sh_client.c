#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int transfer(char *source, char* destination, int size, int* act_size){     //function to transfer data from one array to other and indicating the end of input
    int i=0,flag=1;
    for(i=0;(i<size && source[i]!='\0'); i++){
        destination[i] = source[i];
    }
    if(source[i]=='\0') {
        destination[i] = source[i];
        i++;
        flag =0;
    }
    *act_size = i;
    return flag;
}

int check_if_complete(char *buff, int size){        // checks if the received signal is completely received
    if(buff!=NULL){
        if(buff[size-1]=='\0'){
            return 1;
        }
    }
    return 0;
}

void transfer_recv(char * source, char** destination, int curr_size, int transfer_size){     // fucntion to dynamically create a biggger array to accomodate more data received from teh client
    int i=0,j=0;
    char *temp_buff, *temp_ptr;
    temp_buff = (char*)malloc((curr_size+transfer_size));   // creates a bigger array and copies the content of the previous array
    if(*destination!=NULL){
        for(i=0; i<curr_size; i++){
            temp_buff[i] = (*destination)[i];
        }
        temp_ptr = *destination;
        free(temp_ptr);     // frees the old array
    }
    *destination = temp_buff;       // copies the content of the source array array
    for(j=0; j<transfer_size; j++){
        (*destination)[i] = source[j];
        i++;
    }
}

int main(){
    struct sockaddr_in serv_addr;
    int sockfd;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){       //creates a client side socket
        perror("socket error : socket not created\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;             // filling the feilds of server address
    serv_addr.sin_port = htons(20000);
    inet_aton("127.0.0.1", &serv_addr.sin_addr);

    if(connect(sockfd, (struct sockaddr * )&serv_addr, sizeof(serv_addr))<0){       // connecting to the server having serv_addr address
        perror("connect error : unable to connect to server\n");
        exit(0);
    }

    char *buff,recv_buff[50];
    char *username;
    int uname_size,buff_size;
    long unsigned int size = 10;
    // buff = (char*)malloc(size*sizeof(char));
    buff = NULL;
    char *send_buff,*curr_ptr;
    int client_send_buff_lmt = 50;
    send_buff = (char*)malloc(client_send_buff_lmt*sizeof(char));
    int send_buff_size;
    char *cmd_out;
    cmd_out = NULL;
    int curr_buff_size, recv_bytes;

    curr_buff_size = 0;
    while(1){       // iteratively receives the packets of information and dynamically expands the array
        if(check_if_complete(buff, curr_buff_size)){
            break;
        }
        if((recv_bytes = recv(sockfd, recv_buff, 10, 0))<0){
            perror("receive error : correct message not received\n");
            exit(0);
        }
        transfer_recv(recv_buff, &buff, curr_buff_size, recv_bytes);
        curr_buff_size += recv_bytes;
    }

    printf("%s\n",buff);
    free(buff);
    buff = NULL;
    username = (char *) malloc (size*sizeof(char));
    uname_size = getline (&username, &size, stdin);     // getting the username from the user
    if (uname_size == -1){
        puts ("ERROR!");
    }
    username[uname_size-1] = '\0';
    if(send(sockfd, username, uname_size, 0)!=uname_size){  // sends the NULL terminated username to the server
        perror("send error : sent bytes mismatch\n");
        exit(0);
    }


    curr_buff_size = 0;
    while(1){       // iteratively receives the packets of information and dynamically expands the array
        if(check_if_complete(buff, curr_buff_size)){
            break;
        }
        if((recv_bytes = recv(sockfd, recv_buff, 10, 0))<0){
            perror("receive error : correct message not received\n");
            exit(0);
        }
        transfer_recv(recv_buff, &buff, curr_buff_size, recv_bytes);
        curr_buff_size += recv_bytes;
    }


    if(!strcmp(buff, "NOT-FOUND")){     // if invalid username the exits the program
        close(sockfd);
        printf("Invalid username!!\n");
        exit(0);
    }
    while(1){
        curr_buff_size=0;
        if(cmd_out!=NULL){
            free(cmd_out);
            cmd_out=NULL;
        }
        printf("\nEnter a shell command:\n");
        buff_size = getline (&buff, &size, stdin);     // getting the command from the user
        if (buff_size == -1){
            puts ("ERROR!");
        }
        buff[buff_size-1] = '\0';
        

        curr_ptr = buff;
        while(transfer(curr_ptr, send_buff, client_send_buff_lmt, &send_buff_size)){     // iteratively sending the command to the server in packets
            curr_ptr += send_buff_size;
            if(send(sockfd, send_buff, send_buff_size, 0)!=send_buff_size){
                perror("send error : sent bytes mismatch\n");
                exit(0);
            }
        }
        if(send(sockfd, send_buff, send_buff_size, 0)!=send_buff_size){
            perror("send error : sent bytes mismatch\n");
            exit(0);
        }

        if(!strcmp(buff,"exit")){   // exits if the user wants to exit
            close(sockfd);
            exit(0);
        }

        while(1){       // iteratively receives the packets of information and dynamically expands the array
            if(check_if_complete(cmd_out, curr_buff_size)){
                break;
            }
            if((recv_bytes = recv(sockfd, recv_buff, client_send_buff_lmt, 0))<0){
                perror("receive error : correct message not received\n");
                exit(0);
            }
            transfer_recv(recv_buff, &cmd_out, curr_buff_size, recv_bytes);
            curr_buff_size += recv_bytes;
        }


        if(!strcmp(cmd_out,"$$$$")){    // prints invalid command 
            printf("Invalid command!!\n");
            continue;
        }
        if(!strcmp(cmd_out,"####")){    // prints if there is error in running the code
            printf("Error in running command!!\n");
            continue;
        }
        printf("%s\n",cmd_out); // else prints the command output
    }
    close(sockfd);      // close the socket
    return 0;
}