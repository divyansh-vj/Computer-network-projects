#include "mysocket.h"

pthread_t R,S;
pthread_mutex_t send_lock, recv_lock;
//pthread_cond_t cond_send;
pthread_cond_t cond_recv;
pthread_cond_t cond_send_2, cond_recv_2;
int sockfd;
int newsockfd=-1;
table *Send_Message, *Received_Message;
int min (int a, int b){
    if(a > b){
        return b;
    }
    return a;
}

int check_if_complete(char *buff, int len, int *location){
    for(int i=0;i<len;i++){
        if(buff[i]=='\0'){
            *location = (i+1);
            return 1;
        }
    }
    return 0;
}

void Recv_table_print(){
    printf("\n\nPrinting Receive Table: \n\n");
    printf("\nCount: %d\n", Received_Message->count);
    printf("\nIn: %d\n", Received_Message->in);
    printf("\nOut: %d\n", Received_Message->out);
    for(int i=Received_Message->out; i<Received_Message->in; i++){
        printf("Entry %d: %s\n", (i+1), Received_Message->entry[i]);
    }
    printf("Ending Print\n");
}

void Send_table_print(){
    printf("\n\nPrinting Send Table: \n\n");
    printf("\nCount: %d\n", Send_Message->count);
    printf("\nIn: %d\n", Send_Message->in);
    printf("\nOut: %d\n", Send_Message->out);
    for(int i=Send_Message->out; i<Send_Message->in; i++){
        printf("Entry %d: %s\n", (i+1), Send_Message->entry[i]);
    }
    printf("Ending Print\n");
}

void* myrecv(void* arg){
    while(newsockfd<0);
    int recv_bytes,index=0,size_bytes_rem=4,size=-1;
    int fd = sockfd;
    char buff[4];
    char size_buff[4];
    while(1){
        pthread_mutex_lock(&recv_lock);
        /*
            if newsockfd is -1 then it means that the connection is closed
            so we need to wait for the connection to be established again
            and then we need to reset the variables
        */
        while((Received_Message->count)==10){
            pthread_cond_wait(&cond_recv, &recv_lock);
        } //if Received message table is filled then
        pthread_mutex_unlock(&recv_lock);
        if(newsockfd>0){
            fd = newsockfd;
        }
        while((size_bytes_rem!=0)){
            recv_bytes = recv(fd, buff, size_bytes_rem, 0);
            memcpy(size_buff + (4-size_bytes_rem), buff, recv_bytes);
            size_bytes_rem -= recv_bytes;
        }
        size_bytes_rem = 4;
        sscanf(size_buff, "%d", &size);
        pthread_mutex_lock(&recv_lock);
        Received_Message->size[Received_Message->in] = size;
        while((size!=0)){
            recv_bytes = recv(fd, Received_Message->entry[(Received_Message->in) + index], size, 0);
            index += recv_bytes;
            size -= recv_bytes;
        }
        Received_Message->in = (Received_Message->in+1)%10;
        Received_Message->count++;
        index = 0;
        pthread_cond_broadcast(&cond_recv_2);
        pthread_mutex_unlock(&recv_lock); 
    }
}

void* mysend(void* arg){
    while(newsockfd<0);
    int send_bytes, index = 0;
    int fd = sockfd;
    while(1){
        int flag =0;
        if(newsockfd > 0){
            fd = newsockfd;
        }
        pthread_mutex_lock(&send_lock);
        if(Send_Message->count==0){
            flag = 1;
            // pthread_cond_wait(&cond_send, &send_lock);
        } 
        pthread_mutex_unlock(&send_lock);
        if(flag == 1){
            flag = 0;
            sleep(T);
            continue;
        }
        pthread_mutex_lock(&send_lock);
        int remaining = Send_Message->size[Send_Message->out];
        index = 0;
        char s[5];
        sprintf(s, "%04d", remaining);
        send(fd, s, 4, 0);
        while(remaining != 0){
            if(remaining < 1000){
                send_bytes = send(fd,Send_Message->entry[Send_Message->out] + index, remaining, 0);
                remaining -= send_bytes;
                index += send_bytes;
            }
            else{
                send_bytes = send(fd,Send_Message->entry[Send_Message->out] + index, 1000, 0);
                remaining -= send_bytes;
                index += send_bytes;
            }
        }
        Send_Message->out = (Send_Message->out+1)%10;
        Send_Message->count--;    
        pthread_cond_broadcast(&cond_send_2);
        pthread_mutex_unlock(&send_lock);
        sleep(T);
    }
}

int my_socket(int domain, int type, int protocol){
    if(type!=SOCK_MyTCP){
        printf("mysocket: type is not SOCK_MyTCP");
        return -1;
    }
    pthread_mutex_init(&send_lock, NULL);
    pthread_mutex_init(&recv_lock, NULL);
    //pthread_cond_init(&cond_send, NULL);
    pthread_cond_init(&cond_recv, NULL);
    pthread_cond_init(&cond_recv_2, NULL);
    // pthread_cond_init(&cond_send_2, NULL);
    Send_Message = (table*)malloc(sizeof(table));
    Received_Message = (table*)malloc(sizeof(table));
    Send_Message->count=0;
    Send_Message->in=0;
    Send_Message->out=0;
    Received_Message->count=0;
    Received_Message->in=0;
    Received_Message->out=0;
    sockfd = socket(domain, SOCK_STREAM, protocol);
    if(sockfd<0){
        printf("mysocket: socket error");
        return -1;
    }
    pthread_create(&R,NULL,myrecv,NULL);
    pthread_create(&S,NULL,mysend,NULL);
    return sockfd;
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    if(bind(sockfd, addr, addrlen)<0){   //binds the created socket with an address
        perror("binding error : unable to bind to local address!\n");
        return -1;
    }
    return 0;
}

int my_listen(int sockfd, int backlog){
    if(listen(sockfd, backlog)<0){  //listens for connections on a socket
        perror("listen error : unable to listen on socket!\n");
        return -1;
    }
    return 0;
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    newsockfd = accept(sockfd, addr, addrlen);  //accepts a connection on a socket
    if(newsockfd<0){
        perror("accept error : unable to accept connection!\n");
        return -1;
    }
    return newsockfd;
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    if(connect(sockfd, addr, addrlen)<0){  //connects a socket to a remote address
        perror("connect error : unable to connect to server!\n");
        return -1;
    }
    newsockfd = 0;
    return 0;
}

int my_send(int sockfd, const void *buf, size_t len, int flags){
    pthread_mutex_lock(&send_lock);
    while(Send_Message->count==10){
        pthread_cond_wait(&cond_send_2, &send_lock);
    }
    memcpy(Send_Message->entry[Send_Message->in], buf, len);
    Send_Message->size[Send_Message->in] = len;
    Send_Message->count++;
    Send_Message->in = (Send_Message->in+1)%10;
    // pthread_cond_broadcast(&cond_send);
    pthread_mutex_unlock(&send_lock);
    return len;
}

int my_recv(int sockfd, void *buf, size_t len, int flags){
    pthread_mutex_lock(&recv_lock);
    while(Received_Message->count==0){
        pthread_cond_wait(&cond_recv_2, &recv_lock);
    }
    int min_of_two = min(len, (Received_Message->size[Received_Message->out]));
    memcpy(buf, Received_Message->entry[Received_Message->out], min_of_two);
    Received_Message->count--;
    Received_Message->out = (Received_Message->out+1)%10;
    pthread_cond_broadcast(&cond_recv);
    pthread_mutex_unlock(&recv_lock);
    return min_of_two;
}

int my_close(int fd){

    if((fd==sockfd) && (newsockfd > 0)){
        close(fd);
        return 0;
    }
    sleep(5);
    pthread_cancel(S);
    pthread_cancel(R);
    free(Send_Message);
    free(Received_Message);
    pthread_mutex_destroy(&send_lock);
    pthread_mutex_destroy(&recv_lock);
    pthread_cond_destroy(&cond_recv);
    // pthread_cond_destroy(&cond_send);
    if(close(fd)<0){  //closes a file descriptor
        perror("close error : unable to close socket!\n");
        return -1;
    }
    return 0;
}