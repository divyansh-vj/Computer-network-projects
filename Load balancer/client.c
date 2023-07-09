#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

# define recv_buff_limit 100



int check_if_complete(char *buff, int size){        // checks if the received signal is completely received
    if(buff!=NULL){
        if(buff[size-1]=='\0'){
            return 1;
        }
    }
    return 0;
}

void transfer(char * source, char** destination, int curr_size, int transfer_size){     // fucntion to dynamically create a biggger array to accomodate more data received from teh client
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

int main(int argc, char* argv[]){
    struct sockaddr_in serv_addr;
    char *buff;
    buff = NULL;
    int sockfd;
    int recv_size;
    char recv_buff[recv_buff_limit];
    int serv_port;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){       //creates a client side socket
        perror("socket error : socket not created\n");     
        exit(0);
    }

    serv_addr.sin_family = AF_INET;                         // filling the feilds of server address
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    if(argc==2){
        serv_port = atoi(argv[1]);
        serv_addr.sin_port = htons(serv_port);
    }
    else{
        printf("Expected port no. of server as command line arguements!!\n");
        exit(0);
    }

    if(connect(sockfd, (struct sockaddr *)& serv_addr, sizeof(serv_addr))<0){       // connecting to the server having serv_addr address
        perror("connect error : unable to connect to server\n");
        exit(0);
    }

    int curr_buff_size = 0;
    while(1){       // iteratively receives the packets of information and dynamically expands the array
        if(check_if_complete(buff, curr_buff_size)){
            break;
        }
        if((recv_size = recv(sockfd, recv_buff, recv_buff_limit, 0))<0){
            perror("receive error : correct message not received\n");
            exit(0);
        }
        transfer(recv_buff, &buff, curr_buff_size, recv_size);
        curr_buff_size += recv_size;
    }

    //  printing the date and time
    printf("%s\n", buff);
    

    close(sockfd);      // closing the socket
    
    return 0;
}