#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#define cmd_buff_size 250
#define RECV_BUFF_LIMIT 100
#define SEND_BUFF_LIMIT 50

int get_substr(char *source, char *destination, int *size)
{ // gets the substrings from the bigger string which are seperated by spaces. returns the size of the substring along with setting the flag to 1 if there is nothing after the substring
    int flag = 0;
    char *temp;
    char *dest_ptr;
    dest_ptr = destination;
    temp = source + (*size);
    while (*temp == ' ' || *temp == '\t')
    { // removes empty spaces
        temp++;
        (*size)++;
    }
    while (*temp != ' ' && *temp != '\t' && *temp != '\0' && *temp != '\"')
    { // gets the substring
        *dest_ptr = *temp;
        dest_ptr++;
        temp++;
        (*size)++;
    }
    if (*temp == '\"')
    { // if the command argument has spaces , it handles it. if other flags are included, it treats as invalid command
        temp++;
        (*size)++;
        while (*temp != '\0' && *temp != '\"')
        {
            *dest_ptr = *temp;
            dest_ptr++;
            temp++;
            (*size)++;
        }
        if (*temp == '\"')
        {
            temp++;
            (*size)++;
        }
    }
    while (*temp == ' ' || *temp == '\t')
    { // removes blank spaces after the substring
        temp++;
        (*size)++;
    }
    *dest_ptr = '\0';
    if (*temp == '\0')
    {
        flag = 1;
    }
    return flag;
}

int check_if_complete(char *buff, int size)
{ // checks if the received signal is completely received
    if (buff != NULL)
    {
        if (buff[size - 1] == '\0')
        {
            return 1;
        }
    }
    return 0;
}

void transfer(char *source, char **destination, int curr_size, int transfer_size)
{ // fucntion to dynamically create a biggger array to accomodate more data received from teh client
    int i = 0, j = 0;
    char *temp_buff, *temp_ptr;
    temp_buff = (char *)malloc((curr_size + transfer_size)); // creates a bigger array and copies the content of the previous array
    if (*destination != NULL)
    {
        for (i = 0; i < curr_size; i++)
        {
            temp_buff[i] = (*destination)[i];
        }
        temp_ptr = *destination;
        free(temp_ptr); // frees the old array
    }
    *destination = temp_buff; // copies the content of the source array array
    for (j = 0; j < transfer_size; j++)
    {
        (*destination)[i] = source[j];
        i++;
    }
}

int transfer_send(int n, char *source, char *destination, int size, int *act_size)
{ // function to transfer data from one array to other and indicating the end of input
    int i = 0, flag = 1;
    source += n;
    for (i = 0; (i < size && (*source) != '\0'); i++)
    {
        destination[i] = (*source);
        source++;
    }
    if ((*source) == '\0')
    {
        destination[i] = (*source);
        i++;
        flag = 0;
    }
    *act_size = i;
    return flag;
}

typedef struct header
{
    char *key;
    char *value;
    struct header *next;
} header;

void push(header *top)
{ // push funtion in stack
    header *curr_node;
    curr_node = (header *)malloc(sizeof(header)); // creates a new node
    curr_node->next = top->next;
    top->next = curr_node;
}

void pop(header *top)
{ // pop function of stack
    header *temp;
    temp = top->next;
    top->next = top->next->next;
    free(temp);
}
int process(char **method, char **url, char **protocol_version, header *top, char *buff, int len)
{
    int i = 0, size = 0;
    // int len = strlen(buff);
    int body_len = 0;
    // printf("\n%s\n", buff);
    for (i = 0; i < len; i++)
    {
        if (*method == NULL)
        {
            // printf("method\n");
            int j = i;
            while (buff[j] != ' ')
            {
                size++;
                j++;
            }
            *method = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                (*method)[j - i] = buff[j];
            }
            (*method)[j - i] = '\0';
            i += (size + 1);
            size = 0;

            j = i;
            while (buff[j] != ' ')
            {
                size++;
                j++;
            }
            *url = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                (*url)[j - i] = buff[j];
            }
            (*url)[j - i] = '\0';
            i += (size + 1);
            size = 0;

            j = i;
            while (buff[j] != '\r')
            {
                size++;
                j++;
            }
            *protocol_version = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                (*protocol_version)[j - i] = buff[j];
            }
            (*protocol_version)[j - i] = '\0';
            i += (size - 1);
            size = 0;

            // printf("%s %s %s\n", *method, *url, *protocol_version);
        }
        else if (buff[i] == '\r' && buff[i + 1] == '\n' && buff[i + 2] == '\r' && buff[i + 3] == '\n')
        {
            // i+=4;
            // int j = i;
            // while(buff[j]!='\0'){
            //     size++;
            //     j++;
            // }
            // *body = (char*)malloc((size+1)*sizeof(char));
            // for(j=i;j<(i+size);j++){
            //     (*body)[j-i] = buff[j];
            // }
            // (*body)[j-i] = '\0';
            // i +=size;
            // size = 0;
            break;
        }
        else if (buff[i] == '\r' && buff[i + 1] == '\n')
        {
            // printf("break\n");
            i += 1;
            continue;
        }
        else if(buff[i] == '\r'){
            body_len = -1;
            break;
        }
        else if(buff[i] == '\n'){
            body_len = -1;
            break;
        }
        else
        {
            // printf("header\n");
            // i+=2;
            int j = i;
            while (buff[j] != ':')
            {
                size++;
                j++;
            }
            push(top);
            top->next->key = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                top->next->key[j - i] = buff[j];
            }
            top->next->key[j - i] = '\0';
            // i +=size;
            size = 0;
            j += 2;
            // j++;
            // while(buff[j]==' '){
            //     j++;
            // }
            i = j;
            while (buff[j] != '\r')
            {
                size++;
                j++;
            }
            top->next->value = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                top->next->value[j - i] = buff[j];
            }
            top->next->value[j - i] = '\0';
            if (!strcmp(top->next->key, "Content-Length"))
            {
                body_len = atoi(top->next->value);
            }
            i += (size - 1);
            size = 0;
            // printf("%s %s\n", top->next->key, top->next->value);
            // "a b cs\r\nfd: dsf\r\ndf: dfaa\r\n\r\ndksafklnk";
        }
    }
    // printf("body_len = %d\n", body_len);
    return body_len;
}

int check_body_start(char *buff, int *state, int size)
{
    int i = 0;
    for (i = 0; i < size; i++)
    {
        if (*state == 0)
        {
            if (buff[i] == '\r')
            {
                *state = 1;
            }
        }
        else if (*state == 1)
        {
            if (buff[i] == '\n')
            {
                *state = 2;
            }
            else
            {
                *state = 0;
            }
        }
        else if (*state == 2)
        {
            if (buff[i] == '\r')
            {
                *state = 3;
            }
            else
            {
                *state = 0;
            }
        }
        else if (*state == 3)
        {
            if (buff[i] == '\n')
            {
                *state = 4;
                i++;
                break;
            }
            else
            {
                *state = 0;
            }
        }
    }
    if (*state == 4)
    {
        return (size - i);
    }
    return -1;
}

char * getval(char *key, header *top){
    header *temp = top->next;
    while(temp!=NULL){
        if(!strcmp(temp->key,key)){
            return temp->value;
        }
        temp = temp->next;
    }
    return NULL;
}

int compare(char *str1, char *str2)
{
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0')
    {
        if (str1[i] != str2[i])
        {
            return 0;
        }
        i++;
    }
    if (str1[i] == '\0' && str2[i] == '\0')
    {
        return 1;
    }
    return 0;
}

char *calc_dir_path(char *path){
    int len = strlen(path);
    int flag=0;
    int count=0;
    int i,j;
    for(i=0;i<len;i++){
        if(path[i]=='/')
            break;
    }
    if(i==len) return NULL;
    for(i=0;i<len;i++){
        if(path[i]=='/'){
            for(j=i+1;j<len;j++){
                if(path[j]=='/'){
                    flag=1;
                    break;
                }
            }
            if(flag==1){
                flag=0;
            }
            else{
                break;
            }
        }
        count++;
    }
    char *dir_path;
    dir_path = (char *)malloc((count+1)*sizeof(char));
    for(i=0;i<count;i++){
        dir_path[i] = path[i];
    }
    dir_path[count]= '\0';
    return dir_path;
}


int main()
{
    struct sockaddr_in serv_addr, cli_addr;
    int sockfd, newsockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { // creates a server side socket
        perror("socket error : socket not created\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET; // filling the feilds of server address
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    { // binds the created socket with an address
        perror("binding error : unable to bind to local address!\n");
        exit(0);
    }

    if (listen(sockfd, 5) < 0)
    { // executes the listening call
        perror("listening error \n");
        exit(0);
    }

    char *recv_buff;
    recv_buff = (char *)malloc((RECV_BUFF_LIMIT + 1) * sizeof(char));
    int recv_bytes;

    int cli_num = 1;
    int curr_buff_size = 0;
    unsigned int clilen;
    FILE *fptr;
    fptr = fopen("AccessLog.txt", "w");
    fclose(fptr);

    while (1)
    {

        clilen = sizeof(cli_addr);

        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
        { // creating newsockfd on succressful conntection with the client
            perror("accept error : no client found\n");
            exit(0);
        }
        printf("\n-------------------------client no. %d connected!!-------------------------\n\n\n", cli_num);

        if (fork() == 0)
        { // forking to create a clild process to complete the communication while the parent continues to listen

            char *request;
            request = NULL;
            close(sockfd);
            char file_name[100];
            sprintf(file_name, "client%d.txt", cli_num);
            FILE *fp;
            fp = fopen(file_name, "w");
            fclose(fp);
            fp = fopen(file_name, "a");
            if (fp == NULL)
            {
                perror("file error : unable to open file\n");
                exit(0);
            }
            char *method = NULL, *url = NULL, *protocol_version = NULL;
            // char *body = NULL;
            int body_start = 0, flag = 0, size, max_size;
            // *method = NULL;
            // *url = NULL;
            // *protocol_version = NULL;
            // *body = NULL;
            // printf("hi uper\n");
            header *top;
            top = (header *)malloc(sizeof(header));
            top->next = NULL;
            char cli_IP[INET6_ADDRSTRLEN];
            // printf("hi uper\n");

            char expiry_time[ 100 ] = "";
            time_t rawtime;
            struct tm * timeinfo;
            time(&rawtime);
            timeinfo = gmtime(&rawtime );
            timeinfo->tm_mday += 3;
            mktime(timeinfo);
            strftime(expiry_time, 100, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

            curr_buff_size = 0;
            while (1)
            { // iteratively receives the packets of information and dynamically expands the array
                // if(check_if_complete(request, curr_buff_size)){
                //     break;
                // }
                // printf("hi \n");
                if (flag == 1 && size >= max_size)
                {
                    break;
                }
                if ((recv_bytes = recv(newsockfd, recv_buff, RECV_BUFF_LIMIT, 0)) < 0)
                {
                    perror("receive error : correct message not received\n");
                    exit(0);
                }
                if (flag == 0)
                {
                    size = check_body_start(recv_buff, &body_start, recv_bytes);
                    if (size >= 0)
                    {
                        flag = 1;
                    }
                    transfer(recv_buff, &request, curr_buff_size, recv_bytes);
                    curr_buff_size += recv_bytes;
                    if (flag == 1)
                    {
                        // printf("hi %d \n", size);
                        max_size = process(&method, &url, &protocol_version, top, request, curr_buff_size);
                        if(max_size==-1){
                            char *response = (char *)malloc(200 * sizeof(char));//"HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<html><body><h1>unknown method</h1></body></html>";
                            sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n%s: %d\r\n%s: %s\r\n%s: %s\r\n\r\nInvalid Syntax", "HTTP/1.1", 400, "Bad Request", "Expires", expiry_time, "Cache-control", "no-store", "Content-Length", 14, "Content-Type", "text/*", "Content-language", "en-us");
                            if (send(newsockfd, response, strlen(response), 0) < 0)
                            {
                                perror("send error : unable to send response\n");
                                exit(0);
                            }
                            close(newsockfd);
                            exit(0);
                        }
                        if (max_size >= size)
                        {
                            // printf("hi %d %d\n", size, max_size);
                            int temp = recv_bytes - size;
                            fwrite(recv_buff + temp, sizeof(char), size, fp);
                            // max_size += 1;
                        }
                        else
                        {
                            int temp = recv_bytes - max_size;
                            fwrite(recv_buff + temp, sizeof(char), max_size, fp);
                            // max_size += 1;
                        }
                    }
                }
                else
                {
                    size += recv_bytes;
                    if (size >= max_size)
                    {
                        recv_bytes = max_size - (size - recv_bytes);
                    }
                    recv_buff[recv_bytes] = '\0';
                    fprintf(fp, "%s", recv_buff);
                }
            }

            fclose(fp);
            printf("\n%s %s  %s\n",method, url, protocol_version);
            header *it;
            it = top->next;
            while(it!=NULL){
                printf("%s: %s\n",it->key, it->value);
                it = it->next;
            }
            

            char* host = getval("Host", top);
                if(host == NULL){
                    char *response = (char *)malloc(200 * sizeof(char));//"HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<html><body><h1>unknown method</h1></body></html>";
                    sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n%s: %d\r\n%s: %s\r\n%s: %s\r\n\r\nHost header missing", "HTTP/1.1", 400, "Bad Request", "Expires", expiry_time, "Cache-control", "no-store", "Content-Length", 19, "Content-Type", "text/*", "Content-language", "en-us");
                    if (send(newsockfd, response, strlen(response), 0) < 0)
                    {
                        perror("send error : unable to send response\n");
                        exit(0);
                    }
                    close(newsockfd);
                    exit(0);
                }
                
            
            
            if (!strcmp(method, "GET"))
            {
                fptr = fopen("AccessLog.txt", "a");
                time_t t = time(NULL);
                struct tm date_time = *localtime(&t);
                fprintf(fptr, "%02d/%02d/%d : %02d:%02d:%02d : ", date_time.tm_mday, date_time.tm_mon + 1, date_time.tm_year + 1900, date_time.tm_hour, date_time.tm_min, date_time.tm_sec);
                inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_IP, INET6_ADDRSTRLEN);
                fprintf(fptr, "%s : %d : %s : %s\n", cli_IP, cli_addr.sin_port, "GET", url);
                fclose(fptr);

                char last_mod_time[ 100 ] = "";
                struct stat b;
                if (!stat(url+1, &b)) {
                    strftime(last_mod_time, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtime( &b.st_mtime));
                }
                else{
                    printf("Cannot display the time.\n");
                }

                // printf("hi\n");
                // char expiry_time[ 100 ] = "";
                // time_t rawtime;
                // struct tm * timeinfo, allowed_mod;
                // time(&rawtime);
                // timeinfo = gmtime(&rawtime );
                // timeinfo->tm_mday += 3;
                // mktime(timeinfo);
                // strftime(expiry_time, 100, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

                struct tm allowed_mod;
                // int modified_flag = 0;
                char* modified = getval("If-Modified-Since", top);
                if(modified != NULL){
                    strptime(modified, "%a, %d %b %Y %H:%M:%S GMT", &allowed_mod);
                    if(difftime(mktime(&allowed_mod), mktime(gmtime(&b.st_mtime))) > 0){
                        char *response = (char *)malloc(50 * sizeof(char));
                        sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n\r\n", "HTTP/1.1", 304, "Not Modified", "Expires", expiry_time, "Cache-control", "no-store", "Last-Modified", last_mod_time);
                        int response_size = strlen(response);
                        if (send(newsockfd, response, response_size, 0) < 0)
                        {
                            perror("send error : correct message not sent\n");
                            exit(0);
                        }
                        free(response);
                        close(newsockfd);
                        if(remove(file_name)!=0){
                            printf("Error: unable to delete the file\n");
                        }
                        exit(0);
                    }
                }
                else{
                    char *response = (char *)malloc(200 * sizeof(char));//"HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<html><body><h1>unknown method</h1></body></html>";
                    sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n%s: %d\r\n%s: %s\r\n%s: %s\r\n\r\nIf-Modified-Since header missing", "HTTP/1.1", 400, "Bad Request", "Expires", expiry_time, "Cache-control", "no-store", "Content-Length", 32, "Content-Type", "text/*", "Content-language", "en-us");
                    if (send(newsockfd, response, strlen(response), 0) < 0)
                    {
                        perror("send error : unable to send response\n");
                        exit(0);
                    }
                }
                char *con_type = getval("Accept", top);
                // printf("hi\n");

                // printf("GET request\n");
                FILE *get_file;
                get_file = fopen(url + 1, "r");
                if (get_file == NULL)
                {
                    // printf("File not found\n");
                    char *response = (char *)malloc(50 * sizeof(char));
                    sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n\r\n", "HTTP/1.1", 404, "Not Found", "Expires", expiry_time, "Cache-control", "no-store");
                    int response_size = strlen(response);
                    if (send(newsockfd, response, response_size, 0) < 0)
                    {
                        perror("send error : correct message not sent\n");
                        exit(0);
                    }
                    free(response);
                    close(newsockfd);
                    if(remove(file_name)!=0){
                        printf("Error: unable to delete the file\n");
                    }
                    
                    exit(0);
                }
                fseek(get_file, 0, SEEK_END);
                int file_size = ftell(get_file);
                fseek(get_file, 0, SEEK_SET);
                // printf("hi\n");


                char *response = (char *)malloc(200 * sizeof(char));
                sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n%s: %s\r\n%s: %d\r\n%s: %s\r\n%s: %s\r\n\r\n", "HTTP/1.1", 200, "OK", "Expires", expiry_time, "Cache-control", "no-store", "Content-Language", "en-us", "Content-Length", file_size, "Content-Type", con_type, "Last-Modified", last_mod_time);
                int response_size = strlen(response);
                printf("%s",response);
                if (send(newsockfd, response, response_size, 0) < 0)
                {
                    perror("send error : correct message not sent\n");
                    exit(0);
                }
                // printf("hiaskfdk\n");
                char *file_content;
                file_content = (char *)malloc(SEND_BUFF_LIMIT * sizeof(char));
                int read_size = 0;
                while(1){
                    if((read_size=fread(file_content, sizeof(char), SEND_BUFF_LIMIT, get_file)) != SEND_BUFF_LIMIT){
                        send(newsockfd, file_content, read_size, 0);
                        break;
                    }
                    if (send(newsockfd, file_content, SEND_BUFF_LIMIT, 0) < 0)
                    {
                        perror("send error : correct message not sent\n");
                        exit(0);
                    }
                }
                fclose(get_file);
                free(response);
                free(file_content);
            }
            
            
            
            
            else if(!strcmp(method, "PUT")){
                fptr = fopen("AccessLog.txt", "a");
                time_t t = time(NULL);
                struct tm date_time = *localtime(&t);
                fprintf(fptr, "%02d/%02d/%d : %02d:%02d:%02d : ", date_time.tm_mday, date_time.tm_mon + 1, date_time.tm_year + 1900, date_time.tm_hour, date_time.tm_min, date_time.tm_sec);
                inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_IP, INET6_ADDRSTRLEN);
                fprintf(fptr, "%s : %d : %s : %s\n", cli_IP, cli_addr.sin_port, "PUT", url);
                fclose(fptr);

                

                // char expiry_time[ 100 ] = "";
                // time_t rawtime;
                // struct tm * timeinfo;
                // time(&rawtime);
                // timeinfo = gmtime(&rawtime );
                // timeinfo->tm_mday += 3;
                // mktime(timeinfo);
                // strftime(expiry_time, 100, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);

                
                char *dir_path = calc_dir_path(url+1);
                DIR *dir;
                if(dir_path!=NULL){
                    if((dir = opendir(dir_path)) == NULL){
                        char *response = (char *)malloc(50 * sizeof(char));
                        sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n\r\n", "HTTP/1.1", 404, "Not Found", "Expires", expiry_time, "Cache-control", "no-store");
                        int response_size = strlen(response);
                        if (send(newsockfd, response, response_size, 0) < 0)
                        {
                            perror("send error : correct message not sent\n");
                            exit(0);
                        }
                        free(response);
                        close(newsockfd);
                        if(remove(file_name)!=0){
                            printf("Error: unable to delete the file\n");
                        }
                        exit(0);
                    }
                    closedir(dir);
                }

                if(fork()==0){
                    char *args[4];
                    args[0] = "cp";
                    args[1] = file_name;
                    args[2] = url + 1;
                    args[3] = NULL;
                    execvp(args[0], args);
                }
                else{
                    wait(NULL);
                }


                char *response = (char *)malloc(200 * sizeof(char));
                sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n\r\n", "HTTP/1.1", 200, "OK", "Expires", expiry_time, "Cache-control", "no-store");
                int response_size = strlen(response);
                printf("%s",response);
                if (send(newsockfd, response, response_size, 0) < 0)
                {
                    perror("send error : correct message not sent\n");
                    exit(0);
                }
                free(response);
            }
            
            else
            {
                // printf("invalid request\n");
                char *response = (char *)malloc(200 * sizeof(char));//"HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<html><body><h1>unknown method</h1></body></html>";
                sprintf(response, "%s %d %s\r\n%s: %s\r\n%s: %s\r\n%s: %d\r\n%s: %s\r\n%s: %s\r\n\r\nunknown method", "HTTP/1.1", 400, "Bad Request", "Expires", expiry_time, "Cache-control", "no-store", "Content-Length", 14, "Content-Type", "text/*", "Content-language", "en-us");
                if (send(newsockfd, response, strlen(response), 0) < 0)
                {
                    perror("send error : unable to send response\n");
                    exit(0);
                }
            }
            if(remove(file_name)!=0){
                printf("Error: unable to delete the file\n");
            }
            close(newsockfd); // closing the socket
            // fclose(fp);
            printf("\n------------------------client no. %d disconnected!!------------------------\n\n\n", cli_num);
            exit(0);
        }
        cli_num++;
        close(newsockfd); // closes newsockfd
    }
    return 0;
}
