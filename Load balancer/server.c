#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


#define buff_limit 100


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


int main (int argc, char* argv[]){
    int sockfd, newsockfd;
    unsigned int clilen;
    int sent_amount, buff_size;
    struct sockaddr_in serv_addr, cli_addr;
    char buff[buff_limit];
    char *in_str;
    in_str = NULL;
    int port;
    int random_load;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){   //creates a server side socket
        perror("socket error : socket not created\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;             // filling the feilds of server address
    if(argc==2){
        port = atoi(argv[1]);
        serv_addr.sin_port = htons(port);
    }
    else{
        printf("Expected port no. of server as command line arguements!!\n");
        exit(0);
    }
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){   //binds the created socket with an address
        perror("binding error : unable to bind to local address!\n");
        exit(0);
    }

    if(listen(sockfd, 5)<0){            // executes the listening call
        perror("listening error \n");
        exit(0);
    }

    while(1){
        clilen = sizeof(cli_addr);

        if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen))<0){     //creating newsockfd on succressful conntection with the client
            perror("accept error : no client found\n");
            exit(0);
        }

        int curr_buff_size = 0,recv_size;
        while(1){       // iteratively receives the packets of information and dynamically expands the array
            if(check_if_complete(in_str, curr_buff_size)){
                break;
            }
            if((recv_size = recv(newsockfd, buff, buff_limit, 0))<0){
                perror("receive error : correct message not received\n");
                exit(0);
            }
            transfer(buff, &in_str, curr_buff_size, recv_size);
            curr_buff_size += recv_size;
        }

        time_t t;
        time(&t);
        if(!strcmp(in_str, "Send Load")){   //sends random load
            srand((unsigned) (time(&t)+atoi(argv[1])));
            random_load = ((rand()%100)+1); 
            if((sent_amount = send(newsockfd, &random_load, sizeof(int), 0))!=sizeof(int)){         
                perror("send error : sent bytes mismatch\n");
                exit(0);
            }
            printf("Load sent: %d\n",random_load);
        }
        else if(!strcmp(in_str, "Send Time")){
            strcpy(buff, ctime(&t));
            buff_size = strlen(buff);       // stroring the date and time in the buffer
            buff_size++;
            if((sent_amount = send(newsockfd, buff, buff_size, 0))!=buff_size){         // sending the date and time to the server
                perror("send error : sent bytes mismatch\n");
                exit(0);
            }
        }
        free(in_str);
        in_str = NULL;
        close(newsockfd);    //closing the socket
    }

    return 0;
}