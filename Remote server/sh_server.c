#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<dirent.h>

#define cmd_buff_size 250

int get_substr(char *source, char* destination, int *size){     // gets the substrings from the bigger string which are seperated by spaces. returns the size of the substring along with setting the flag to 1 if there is nothing after the substring
    int flag = 0;
    char *temp;
    char* dest_ptr;
    dest_ptr = destination;
    temp = source + (*size);
    while(*temp==' '||*temp=='\t') {    //removes empty spaces
        temp++;
        (*size)++;
    }
    while(*temp!=' ' && *temp!='\t' && *temp!='\0' && *temp!='\"'){ // gets the substring
        *dest_ptr = *temp;
        dest_ptr++;
        temp++;
        (*size)++;
    }
    if(*temp=='\"'){    //if the command argument has spaces , it handles it. if other flags are included, it treats as invalid command
        temp++;
        (*size)++;
        while(*temp!='\0' && *temp!='\"'){
            *dest_ptr = *temp;
            dest_ptr++;
            temp++;
            (*size)++;
        }
        if(*temp=='\"'){
            temp++;
            (*size)++;
        }
    }
    while(*temp==' '||*temp=='\t') {    //removes blank spaces after the substring
        temp++;
        (*size)++;
    }
    *dest_ptr = '\0';
    if(*temp=='\0'){
        flag=1;
    }
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


int transfer_send(int n,char *source, char* destination, int size, int* act_size){     //function to transfer data from one array to other and indicating the end of input
    int i=0,flag=1;
    source +=n;
    for(i=0;(i<size && (*source)!='\0'); i++){
        destination[i] = (*source);
        source++;
    }
    if((*source)=='\0') {
        destination[i] = (*source);
        i++;
        flag =0;
    }
    *act_size = i;
    return flag;
}


int main(){
    struct sockaddr_in serv_addr, cli_addr;
    int sockfd, newsockfd;



    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){       //creates a server side socket
        perror("socket error : socket not created\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;         // filling the feilds of server address
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);

    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){   //binds the created socket with an address
        perror("binding error : unable to bind to local address!\n");
        exit(0);
    }

    if(listen(sockfd, 5)<0){        // executes the listening call
        perror("listening error \n");
        exit(0);
    }

    char *username, file_uname[26], *command, temp_cmd[50], *recv_buff, *send_buff, buff[10];
    int recv_buff_limit = 50;
    int send_buff_size;
    recv_buff = (char*)malloc(recv_buff_limit*sizeof(char));
    send_buff = (char*)malloc(recv_buff_limit*sizeof(char));
    int recv_bytes;
    command = NULL;
    username = NULL;
    int size=0,flag;
    int cli_num=1;
    buff[0] = '\0';
    strcat(buff, "LOGIN");
    int curr_buff_size=0;
    unsigned int clilen;


    while(1){

        clilen = sizeof(cli_addr);

        if((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen))<0){      //creating newsockfd on succressful conntection with the client
            perror("accept error : no client found\n");
            exit(0);
        }
        printf("\n-------------------------client no. %d connected!!-------------------------\n\n\n",cli_num);

        if(fork()==0){      //forking to create a clild process to complete the communication while the parent continues to listen

            close(sockfd);
            FILE* fptr;
            int found = 0;
            if(send(newsockfd, buff, strlen(buff)+1, 0)!=(strlen(buff)+1)){
                perror("send error : sent bytes mismatch\n");
                exit(0);
            }
            curr_buff_size = 0;
            while(1){       // iteratively receives the packets of information and dynamically expands the array
                if(check_if_complete(username, curr_buff_size)){
                    break;
                }
                if((recv_bytes = recv(newsockfd, recv_buff, recv_buff_limit, 0))<0){
                    perror("receive error : correct message not received\n");
                    exit(0);
                }
                transfer(recv_buff, &username, curr_buff_size, recv_bytes);
                curr_buff_size += recv_bytes;
            }




            if((fptr=fopen("users.txt","r"))==NULL){    //checks for the username in user.txt
                printf("no file with usernames (users.txt)!!\n");
                username[0] = '\0';
                strcat(username,"NOT-FOUND");
                if(send(newsockfd, username, strlen(username)+1, 0)!=(strlen(username)+1)){
                    perror("send error : sent bytes mismatch\n");
                    exit(0);
                }
                close(newsockfd);
                printf("\n------------------------client no. %d disconnected!!------------------------\n\n\n",cli_num);
                exit(0);
            }

            while(fgets(file_uname, 26, fptr)!=NULL){   //sends FOUND on successfully finding the username
                file_uname[strlen(file_uname)-1] = '\0';
                if(!strcmp(file_uname, username)){
                    fclose(fptr);
                    found = 1;
                    username[0] = '\0';
                    strcat(username,"FOUND");
                    if(send(newsockfd, username, strlen(username)+1, 0)!=(strlen(username)+1)){
                        perror("send error : sent bytes mismatch\n");
                        exit(0);
                    }
                    break;
                }
            }

            if(found==0){
                fclose(fptr);
                username[0] = '\0';     //sends NOT-FOUND otherwise
                strcat(username,"NOT-FOUND");
                if(send(newsockfd, username, strlen(username)+1, 0)!=(strlen(username)+1)){
                    perror("send error : sent bytes mismatch\n");
                    exit(0);
                }
                close(newsockfd);
                printf("\n------------------------client no. %d disconnected!!------------------------\n\n\n",cli_num);
                exit(0);
            }


            char *cmd_out;
            char *temp_buff;
            while(1){
                cmd_out = temp_buff = NULL;
                cmd_out = (char*)malloc(cmd_buff_size*sizeof(char));
                cmd_out[0]='\0';
                size = 0;
                int curr_size=cmd_buff_size;
                
                if(command!=NULL){
                    free(command);
                    command=NULL;
                }


                curr_buff_size = 0;
                while(1){       // iteratively receives the packets of information and dynamically expands the array
                    if(check_if_complete(command, curr_buff_size)){
                        break;
                    }
                    if((recv_bytes = recv(newsockfd, recv_buff, recv_buff_limit, 0))<0){
                        perror("receive error : correct message not received\n");
                        exit(0);
                    }
                    transfer(recv_buff, &command, curr_buff_size, recv_bytes);
                    curr_buff_size += recv_bytes;
                }
                if(!strcmp(command,"exit")){    //closes the connection on exit
                    break;
                }
                flag = get_substr(command, temp_cmd, &size);
                if(!strcmp(temp_cmd,"pwd")){    //executes pwd command

                    if(flag==0){    //if some other arguement then return error
                        cmd_out[0]='\0';
                        strcat(cmd_out,"$$$$");
                    }
                    else if(getcwd(cmd_out,cmd_buff_size)==NULL){ //gets current working directry
                            cmd_out[0]='\0';
                            strcat(cmd_out,"####");
                        }
                    
                        
                }

                else if(!strcmp(temp_cmd,"cd")){    //executes cd

                    if(flag==1){
                        if(chdir(getenv("HOME"))!=0){   //changes to home directry if no arguements
                            cmd_out[0]='\0';
                            strcat(cmd_out,"####");
                        }
                        else{
                            cmd_out[0]='\0';
                            strcat(cmd_out,"cd command successful");
                        }
                    }
                    else{
                        flag = get_substr(command, temp_cmd, &size);    //esle changes to required directry
                        if(flag==0){
                            cmd_out[0]='\0';
                            strcat(cmd_out,"$$$$");
                        }
                        else {
                            if(chdir(temp_cmd)!=0){
                                cmd_out[0]='\0';
                                strcat(cmd_out,"####");
                            }
                            else{
                                cmd_out[0]='\0';
                                strcat(cmd_out,"cd command successful");
                            }
                        }

                    }
                }
                else if(!strcmp(temp_cmd, "dir")){  //executes dir command 
                    DIR *dir;
                    struct dirent* entry;
                    int len;
                    dir = NULL;
                    if(flag==1){
                        if((dir=opendir("."))==NULL){   //open current directry if no arguements
                            cmd_out[0]='\0';
                            strcat(cmd_out,"####");
                        }
                    }
                    else{
                        flag = get_substr(command, temp_cmd, &size);
                        if(flag==0){
                            cmd_out[0]='\0';
                            strcat(cmd_out,"$$$$");
                        }
                        else{
                            if((dir=opendir(temp_cmd))==NULL){  //else open required directory
                                cmd_out[0]='\0';
                                strcat(cmd_out,"####");
                            }
                        }
                    }
                    if(dir!=NULL){
                        cmd_out[0]='\0';
                        while((entry=readdir(dir))!=NULL){
                            if((strlen(cmd_out)+strlen(entry->d_name))>=(curr_size-2)){  //reads the contents of the directory and concatenates it in the string
                                char *temp_cmd_out;
                                temp_cmd_out = (char*)malloc(strlen(cmd_out)+cmd_buff_size);
                                for(int j=0;j<=strlen(cmd_out);j++){
                                    temp_cmd_out[j] = cmd_out[j];
                                }
                                curr_size+=cmd_buff_size;
                                free(cmd_out);
                                cmd_out = temp_cmd_out;
                                temp_cmd_out=NULL;
                            }
                            strcat(cmd_out, entry->d_name);
                            len = strlen(cmd_out);
                            cmd_out[len] = '\n';
                            cmd_out[len+1] = '\0';
                        }
                    }
                    closedir(dir);
                }
                else{
                    strcat(cmd_out,"$$$$");
                }
                int n=0;
                while(transfer_send(n,cmd_out, send_buff, recv_buff_limit, &send_buff_size)){     // iteratively sending the data to the server in packets
                    n += send_buff_size;
                    if(send(newsockfd, send_buff, send_buff_size, 0)!=send_buff_size){
                        perror("send error : sent bytes mismatch\n");
                        exit(0);
                    }
                }

                if(send(newsockfd, send_buff, send_buff_size, 0)!=send_buff_size){
                    perror("send error : sent bytes mismatch\n");
                    exit(0);
                }
                if(cmd_out!=NULL){
                    free(cmd_out);
                    cmd_out=NULL;
                }
                if(temp_buff!=NULL){
                    free(temp_buff);
                    temp_buff=NULL;
                }
            }
            close(newsockfd);   // closing the socket
            printf("\n------------------------client no. %d disconnected!!------------------------\n\n\n",cli_num);
            exit(0);
        }

        cli_num++;
        close(newsockfd);   //closes newsockfd

    }
    return 0;
}