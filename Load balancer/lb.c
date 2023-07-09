#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>

#define num_server 2
#define recv_buff_limit 100


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
    int sockfd, newsockfd, temp_sockfd;
    unsigned int clilen;
    struct sockaddr_in serv_addr[num_server], cli_addr, lb_addr;
    int port_serv, port_lb;
    int serv_load[num_server];
    time_t st_time, end_time;
    char send_str[100];
    int poll_wait=0;
    char serv_IP[INET6_ADDRSTRLEN];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){   //creates a server side socket
        perror("socket error : socket not created\n");
        exit(0);
    }

    lb_addr.sin_family = AF_INET;
    lb_addr.sin_addr.s_addr = INADDR_ANY;   //filling the address of the load balancer

    if(argc==(num_server+2)){
        port_lb = atoi(argv[1]);
        lb_addr.sin_port = htons(port_lb);
        for(int i=2;i<(num_server+2);i++){
            serv_addr[i-2].sin_family = AF_INET;             // filling the feilds of server address
            inet_aton("127.0.0.1", &serv_addr[i-2].sin_addr);
            port_serv = atoi(argv[i]);
            serv_addr[i-2].sin_port = htons(port_serv);
        }
    }
    else{
        printf("Expected port no. of load balancer, server1 and server2 as command line arguements!!\n");
        exit(0);
    }


    if(bind(sockfd, (struct sockaddr *)&lb_addr, sizeof(lb_addr))<0){   //binds the created socket with an address
        perror("binding error : unable to bind to local address!\n");
        exit(0);
    }

    if(listen(sockfd, 5)<0){            // executes the listening call
        perror("listening error \n");
        exit(0);
    }


    while(1){
        int poll_ret;
        struct pollfd fdset[1];
        fdset[0].fd = sockfd;
        fdset[0].events = POLLIN;

        time(&st_time);        
        poll_ret = poll(fdset, 1, poll_wait);
        time(&end_time);
        if(poll_ret==0){    //on timeout asks the servers for load
            strcpy(send_str,"Send Load");
            for(int i=0;i<num_server;i++){
                if((temp_sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){   //creates a temporary socket to talk with servers
                    perror("socket error : socket not created\n");
                    exit(0);
                }
                if(connect(temp_sockfd, (struct sockaddr *)& serv_addr[i], sizeof(serv_addr[i]))<0){       // connecting to the server having serv_addr address
                    perror("connect error : unable to connect to server\n");
                    exit(0);
                }
                if(send(temp_sockfd, send_str, (strlen(send_str)+1), 0)!=(strlen(send_str)+1)){         
                    perror("send error : sent bytes mismatch\n");
                    exit(0);
                }
                if(recv(temp_sockfd, &serv_load[i], sizeof(int), 0)<0){
                    perror("receive error : correct message not received\n");
                    exit(0);
                }
                close(temp_sockfd);
                inet_ntop(AF_INET, &(serv_addr[i].sin_addr), serv_IP, INET6_ADDRSTRLEN);
                printf("Load received from %s: %d\n\n",serv_IP,serv_load[i]);    
            }
            poll_wait=5000;
        }
        else if(poll_ret>0){
            clilen = sizeof(cli_addr);
            if((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen))<0){      //creating newsockfd on succressful conntection with the client
                perror("accept error : no client found\n");
                exit(0);
            }
            if(fork()==0){
                close(sockfd);
                int min_load_serv, min_load=__INT_MAX__;
                char* recvd_time;
                recvd_time = NULL;
                int curr_buff_size = 0,recv_size;
                char recv_buff[recv_buff_limit];
                for(int i=0;i<num_server;i++){
                    if(serv_load[i]<min_load){
                        min_load = serv_load[i];
                        min_load_serv=i;
                    }
                }
                inet_ntop(AF_INET, &(serv_addr[min_load_serv].sin_addr), serv_IP, INET6_ADDRSTRLEN);
                printf("Sending client request to %s\n\n",serv_IP);
                strcpy(send_str,"Send Time");
                if((temp_sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){   //creates a temporary socket
                    perror("socket error : socket not created\n");
                    exit(0);
                }
                if(connect(temp_sockfd, (struct sockaddr *)& serv_addr[min_load_serv], sizeof(serv_addr[min_load_serv]))<0){       // connecting to the server having serv_addr address
                    perror("connect error : unable to connect to server\n");
                    exit(0);
                }
                if(send(temp_sockfd, send_str, (strlen(send_str)+1), 0)!=(strlen(send_str)+1)){         // getting the date and time from the server
                    perror("send error : sent bytes mismatch\n");
                    exit(0);
                }
                while(1){       // iteratively receives the packets of information and dynamically expands the array
                    if(check_if_complete(recvd_time, curr_buff_size)){
                        break;
                    }
                    if((recv_size = recv(temp_sockfd, recv_buff, recv_buff_limit, 0))<0){
                        perror("receive error : correct message not received\n");
                        exit(0);
                    }
                    transfer(recv_buff, &recvd_time, curr_buff_size, recv_size);
                    curr_buff_size += recv_size;
                }
                close(temp_sockfd);
                if(send(newsockfd, recvd_time, (strlen(recvd_time)+1), 0)!=(strlen(recvd_time)+1)){         // sending the date and time to the client
                    perror("send error : sent bytes mismatch\n");
                    exit(0);
                }
                free(recvd_time);
                recvd_time = NULL;
                close(newsockfd);
                exit(0);
            }
            close(newsockfd); // parent process returns to the main loop
            poll_wait = (5-(end_time-st_time))*1000;
        }
        
    }
    close(sockfd);
    return 0;
}
