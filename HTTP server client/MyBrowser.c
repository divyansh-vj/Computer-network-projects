
/*    THE CLIENT PROCESS */


#include <stdio.h>
#include<sys/poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<time.h>

#define cmd_buff_size 250
#define RECV_BUFF_LIMIT 100
#define SEND_BUFF_LIMIT 100


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

typedef struct header{
    char *key;
    char *value;
    struct header *next;
}header;

void push(header *top){        // push funtion in stack
    header* curr_node;
    curr_node = (header*)malloc(sizeof(header));  //creates a new node
    curr_node->next = top->next;
    top->next = curr_node;
}

void pop(header *top){     // pop function of stack
    header* temp;
    temp = top->next;
    top->next = top->next->next;
    free(temp);
}
int process(char **protocol_version, char **status_code, char **status_msg, header *top, char *buff, int len)
{
    int i = 0, size = 0;
    // int len = strlen(buff);
    int body_len = 0;
    // printf("\n%s\n", buff);
    for (i = 0; i < len; i++)
    {
        if (*protocol_version == NULL)
        {
            int j = i;
            while (buff[j] != ' ')
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
            i += (size + 1);
            size = 0;

            j = i;
            while (buff[j] != ' ')
            {
                size++;
                j++;
            }
            *status_code = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                (*status_code)[j - i] = buff[j];
            }
            (*status_code)[j - i] = '\0';
            i += (size + 1);
            size = 0;

            j = i;
            while (buff[j] != '\r')
            {
                size++;
                j++;
            }
            *status_msg = (char *)malloc((size + 1) * sizeof(char));
            for (j = i; j < (i + size); j++)
            {
                (*status_msg)[j - i] = buff[j];
            }
            (*status_msg)[j - i] = '\0';
            i += (size - 1);
            size = 0;
        }
        else if (buff[i] == '\r' && buff[i + 1] == '\n' && buff[i + 2] == '\r' && buff[i + 3] == '\n')
        {
            break;
        }
        else if (buff[i] == '\r' && buff[i + 1] == '\n')
        {
            i += 1;
            continue;
        }

        else
        {
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
        }
    }
    return body_len;
}

char * header_finder(header * top, char *key){
	char * value = (char *)malloc(1000 *sizeof(char));
	while(strcmp(top->next->key, key)){
		top = top->next;
	}
	memset(value, '\0', sizeof(value));
	strcpy(value, top->next->value);
	return value;
}

int check_body_start(char *buff, int *state, int size){
    int i=0;
    for(i=0;i<size;i++){
        if(*state == 0){
            if(buff[i]=='\r'){
                *state = 1;
            }
        }
        else if(*state == 1){
            if(buff[i]=='\n'){
                *state = 2;
            }
            else{
                *state = 0;
            }
        }
        else if(*state == 2){
            if(buff[i]=='\r'){
                *state = 3;
            }
            else{
                *state = 0;
            }
        }
        else if(*state == 3){
            if(buff[i]=='\n'){
                *state = 4;
                i++;
                break;
            }
            else{
                *state = 0;
            }
        }
    }
    if(*state == 4){
        return (size-i);
    }
    return -1;
}

int min(int a, int b){
	return (a > b) ? b:a;
}

struct tm * addinterval(struct tm * x, int y, int m, int d) {
    x->tm_year += y;
    x->tm_mon += m;
    x->tm_mday += d;
    mktime(x); /* normalize result */
    return x;
}

char * GET_req(char *host, char *url, char* port){
	char * http = (char *)malloc(10000*sizeof(char));
	http[0] = '\0';
	strcat(http,"GET ");
	strcat(http, url);
	strcat(http, " HTTP/1.1\r\n");
	strcat(http, "Host: ");
	strcat(http, host);
	strcat(http, "\r\nAccept: ");
	//printf("%s", http);
	char *accept = (char *)malloc(100*sizeof(char));
	int len = strlen(url);
	if(url[len-4] == 'h' && url[len-3] == 't' && url[len-2] == 'm' && url[len-1] == 'l'){
		accept = "text/html\r\n";
	}
	else if(url[len-3] == 'p' && url[len-2] == 'd' && url[len-1] == 'f'){
		accept = "application/pdf\r\n";
	}
	else if(url[len-3] == 'j' && url[len-2] =='p' && url[len-1] == 'g'){
		accept = "image/jpeg\r\n";
	}
	else{
		accept = "text/*\r\n";
	}
	http = strcat(http, accept);
	http = strcat(http, "Accept-Language: en-us\r\n");
	time_t rawtime;
	struct tm * timeinfo;

   	time(&rawtime);
   	timeinfo = gmtime(&rawtime );
	timeinfo = addinterval(timeinfo, 0, 0, -2);
	http = strcat(http, "If-Modified-Since: ");
	switch (timeinfo->tm_wday){
		case 0:
			http = strcat(http, "Mon, ");
			break;
		case 1:
			http = strcat(http, "Tue, ");
			break;
		case 2:
			http = strcat(http, "Wed, ");
			break;
		case 3:
			http = strcat(http, "Thu, ");
			break;
		case 4:
			http = strcat(http, "Fri, ");
			break;
		case 5:
			http = strcat(http, "Sat, ");
			break;
		case 6:
			http = strcat(http, "Sun, ");
			break;
	}
	char *day;
	day = (char *)malloc(10*sizeof(char));
	sprintf(day, "%d", (timeinfo->tm_mday));
	http = strcat(http, day);
	switch (timeinfo->tm_mon){
		case 0:
			http = strcat(http, " Jan ");
			break;
		case 1:
			http = strcat(http, " Feb ");
			break;
		case 2:
			http = strcat(http, " Mar ");
			break;
		case 3:
			http = strcat(http, " Apr ");
			break;
		case 4:
			http = strcat(http, " May ");
			break;
		case 5:
			http = strcat(http, " Jun ");
			break;
		case 6:
			http = strcat(http, " Jul ");
			break;
		case 7:
			http = strcat(http, " Aug ");
			break;
		case 8:
			http = strcat(http, " Sep ");
			break;
		case 9:
			http = strcat(http, " Oct ");
			break;
		case 10:
			http = strcat(http, " Nov ");
			break;
		case 11:
			http = strcat(http, " Dec ");
			break;
	}
	char *year;
	year = (char *)malloc(10*sizeof(char));
	sprintf(year, "%d", (timeinfo->tm_year)+1900);
	http = strcat(http, year);
	char *time;
	time = (char *)malloc(100*sizeof(char));
	sprintf(time, " %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	http = strcat(http, time);
	http = strcat(http, " GMT\r\n");
	http = strcat(http, "\r\n");
	return http;
}

char *PUT_req(char *host, char *url, char* port, char*file){
	char * http = (char *)malloc(10000*sizeof(char));
	http[0] = '\0';
	strcat(http,"PUT ");
	strcat(url, "/");
	strcat(url, file);
	strcat(http, url);
	strcat(http, " HTTP/1.1\r\n");
	strcat(http, "Host: ");
	strcat(http, host);
	strcat(http, "\r\nAccept: ");
	//printf("%s", http);
	char *accept = (char *)malloc(100*sizeof(char));
	int len = strlen(file);
	if(file[len-4] == 'h' && file[len-3] == 't' && file[len-2] == 'm' && file[len-1] == 'l'){
		accept = "text/html\r\n";
	}
	else if(file[len-3] == 'p' && file[len-2] == 'd' && file[len-1] == 'f'){
		accept = "application/pdf\r\n";
	}
	else if(file[len-3] == 'j' && file[len-2] =='p' && file[len-1] == 'g'){
		accept = "image/jpeg\r\n";
	}
	else{
		accept = "text/*\r\n";
	}
	http = strcat(http, accept);
	http = strcat(http, "Accept-Language: en-us\r\n");
	time_t rawtime;
	struct tm * timeinfo;

   	time(&rawtime);
   	timeinfo = gmtime(&rawtime );
	timeinfo = addinterval(timeinfo, 0, 0, -2);
	http = strcat(http, "If-modified-since: ");
	switch (timeinfo->tm_wday){
		case 0:
			http = strcat(http, "Mon, ");
			break;
		case 1:
			http = strcat(http, "Tue, ");
			break;
		case 2:
			http = strcat(http, "Wed, ");
			break;
		case 3:
			http = strcat(http, "Thu, ");
			break;
		case 4:
			http = strcat(http, "Fri, ");
			break;
		case 5:
			http = strcat(http, "Sat, ");
			break;
		case 6:
			http = strcat(http, "Sun, ");
			break;
	}
	char *day;
	day = (char *)malloc(10*sizeof(char));
	sprintf(day, "%d", (timeinfo->tm_mday));
	http = strcat(http, day);
	switch (timeinfo->tm_mon){
		case 0:
			http = strcat(http, " Jan ");
			break;
		case 1:
			http = strcat(http, " Feb ");
			break;
		case 2:
			http = strcat(http, " Mar ");
			break;
		case 3:
			http = strcat(http, " Apr ");
			break;
		case 4:
			http = strcat(http, " May ");
			break;
		case 5:
			http = strcat(http, " Jun ");
			break;
		case 6:
			http = strcat(http, " Jul ");
			break;
		case 7:
			http = strcat(http, " Aug ");
			break;
		case 8:
			http = strcat(http, " Sep ");
			break;
		case 9:
			http = strcat(http, " Oct ");
			break;
		case 10:
			http = strcat(http, " Nov ");
			break;
		case 11:
			http = strcat(http, " Dec ");
			break;
	}
	char *year;
	year = (char *)malloc(10*sizeof(char));
	sprintf(year, "%d", (timeinfo->tm_year)+1900);
	http = strcat(http, year);
	char *time;
	time = (char *)malloc(100*sizeof(char));
	sprintf(time, " %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	http = strcat(http, time);
	http = strcat(http, " GMT\r\n");
	strcat(http, "Content-Language: en-us\r\n");
	long long int bytes = 0;
	FILE *fp = fopen(file,"r");
	while(fgetc(fp) != EOF) bytes++; 
	strcat(http, "Content-Length: ");
	char *byt;
	byt = (char*)malloc(1000*sizeof(char));
	sprintf(byt, "%lld", bytes);
	strcat(http, byt);
	strcat(http, "\r\nContent-Type: ");
	strcat(http, accept);
	strcat(http, "\r\n\r\n");
	return http;
}

int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;
	int i;


	/* Recall that we specified INADDR_ANY when we specified the server
	   address in the server. Since the client can run on a different
	   machine, we must specify the IP address of the server. 

	   In this program, we assume that the server is running on the
	   same machine as the client. 127.0.0.1 is a special address
	   for "localhost" (this machine)
	   
	/* IF YOUR SERVER RUNS ON SOME OTHER MACHINE, YOU MUST CHANGE 
           THE IP ADDRESS SPECIFIED BELOW TO THE IP ADDRESS OF THE 
           MACHINE WHERE YOU ARE RUNNING THE SERVER. 
    	*/

	/* With the information specified in serv_addr, the connect()
	   system call establishes a connection with the server process.
	*/

	while(1){
		int err;
		char *command, *buf, *host, *url, *port, *http, *file;
		command = (char *)malloc(10000*sizeof(char));
		buf = (char *)malloc(10000*sizeof(char));
		host = (char *)malloc(10000*sizeof(char));
		url = (char *)malloc(10000*sizeof(char));
		http = (char *)malloc(10000*sizeof(char));
		file = (char *)malloc(10000*sizeof(char));
		port = (char *)malloc(100*sizeof(char));
		printf("\nMyOwnBrowser> ");
		scanf("%[^\n]s", command);
		getchar();
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("Unable to create socket\n");
			exit(0);
		}
		struct pollfd fd_set[1];
		fd_set[0].fd = sockfd;
		fd_set[0].events = POLLIN;
		int ret = poll(fd_set, 1, 3000);
		if(ret == 0){
			printf("Timeout Reached\n");
			close(sockfd);
			continue;
		}
		if(command[0] == 'G' && command[1] == 'E' && command[2] == 'T'){
			memset(buf, '\0', sizeof(buf));
			memset(port, '\0', sizeof(port));
			strncpy(buf, command+4, strlen(command)-4);
			buf[strlen(buf)] = '\0';
			//printf("\nbuf: %s END\n", buf);
			err = sscanf( buf, "%*[^/]%*[/]%[^/]", host);
			err = sscanf(buf, "%*[^/]%*[/]%*[^/]%[^:]", url);
			err = sscanf(buf, "%*[^/]%*[/]%*[^/]%*[^:]%*[:]%s", port);
			//printf("HOST: %s  url: %s  port: %s\n", host, url, port);
			http = GET_req(host, url, port);
			//printf("\n\n%s", http);
			int p_no;
			if(strlen(port) == 0){
				p_no = 80;
			}
			else{
				p_no = atoi(port);
			}
			//printf("\nPORT: %d\n, HOST: %s", p_no, host);
			serv_addr.sin_family	= AF_INET;
			inet_aton(host, &serv_addr.sin_addr);
			serv_addr.sin_port	= htons(p_no);
			if ((connect(sockfd, (struct sockaddr *) &serv_addr,
								sizeof(serv_addr))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}
			int size = strlen(http);
			for(int i=0; i<=size; i+=50){
				for(int j=0; j<50; j++){buf[j] = '\0';}
				if(i == size){
					send(sockfd, buf, strlen(buf) + 1, 0);
					continue;
				}
				strncpy(buf, http + i, min(size - i, 50));
				if(size - i >= 50){
					send(sockfd, buf, 50, 0);
				}
				else{
					send(sockfd, buf, size-i, 0);
				}
			}
			char *recv_buff;
			recv_buff = (char*)malloc((RECV_BUFF_LIMIT+1)*sizeof(char));
			int recv_bytes;

			int cli_num=1;
			int curr_buff_size=0;
			unsigned int clilen;
			char *request;
			request = NULL;
			char file_name[100];
            FILE *fp;
			fp = fopen("out.txt", "w");
            if(fp==NULL){
                perror("file error : unable to open file\n");
                exit(0);
            }
            char *status_code = NULL, *status_msg = NULL, *protocol_version = NULL;
            // char *body = NULL;
            int body_start = 0,flag=0,max_size;
            header *top;
            top = (header *)malloc(sizeof(header));
            top->next=NULL;
            
            curr_buff_size = 0;
            while(1){       // iteratively receives the packets of information and dynamically expands the array
			                // if(check_if_complete(request, curr_buff_size)){
                //     break;
                // }
                //printf("hi \n");
                if(flag==1 && size>=max_size){
                    break;
                }
                if((recv_bytes = recv(sockfd, recv_buff, RECV_BUFF_LIMIT, 0))<0){
                    perror("receive error : correct message not received\n");
                    exit(0);
                }
				// FILE*fpr;
				// fpr = fopen("checking.txt", "w");
				// fprintf(fpr, "%s", recv_buff);
				//printf("%s", recv_buff);
                if(flag==0){
                    size = check_body_start(recv_buff, &body_start, recv_bytes);
                    if(size>=0){
                        flag = 1;
						//printf("\n\n Flag starts here: %s\nEND", recv_buff);
                    }
                    transfer(recv_buff, &request, curr_buff_size, recv_bytes);
                    curr_buff_size += recv_bytes;
                    if(flag == 1){
                            //printf("hi %d \n",size );
                        max_size = process(&protocol_version, &status_code, &status_msg, top, request, curr_buff_size);
						//header_finder(top, "Content-Type");
                        if(max_size>= size){
                            //printf("hi %d %d\n",size, max_size);
                            int temp = recv_bytes - size;
							//printf("\nChecking: %s\nENDING\nSIZE: %d\n", recv_buff+temp, size);
                            int ret = fwrite(recv_buff+temp, sizeof(char), size, fp);
							//printf("\n\nReturn Value of fwrite: %d\nENDING",ret);
                            // max_size += 1;
                        }
                        else{
                            int temp = recv_bytes - max_size;
							//printf("\nChecking2: %s\nENDING", recv_buff+temp);
                            fwrite(recv_buff+temp, sizeof(char), max_size, fp);
                            // max_size += 1;
                        }
                    }
                }
                else{
                    size += recv_bytes;
                    if(size>=max_size){
                        recv_bytes = max_size - (size - recv_bytes);
                    }
                    recv_buff[recv_bytes] = '\0';
                    fprintf(fp, "%s", recv_buff);
                }
            }
			if(!strcmp(status_code, "200")){;}
			else if(!strcmp(status_code, "400")){
				printf("ERROR: HTTP 400 BAD REQUEST\n");
			}
			else if(!strcmp(status_code, "403")){
				printf("ERROR: HTTP 403 FORBIDDEN\n");
			}
			else if(!strcmp(status_code, "404")){
				printf("ERROR: HTTP 404 NOT FOUND\n");
			}
			else{
				printf("UNKNOWN ERROR: HTTP %s\n",status_code);
			}
			fclose(fp);
			printf("\n%s %s  %s\n",protocol_version, status_code, status_msg);
            header *it;
            it = top->next;
            while(it!=NULL){
                printf("%s: %s\n",it->key, it->value);
                it = it->next;
            }
			char *type = (char *)malloc(1000*sizeof(char));
			type = header_finder(top, "Content-Type");
			char *check = (char *)malloc(16*sizeof(char)), filename[100] = "out.txt";
			strncpy(check, type, 9);
			check[9] = '\0';
			if(!strcmp(check, "text/html")){
				strcpy(filename,"out.html");
			}
			strncpy(check, type, 15);
			check[15] = '\0';
			if(!strcmp(check, "application/pdf")){
				strcpy(filename,"out.pdf");
			}
			strncpy(check, type, 10);
			check[10] = '\0';
			if(!strcmp(check, "image/jpeg")){
				strcpy(filename,"out.jpg");
			}
			strncpy(check, type, 6);
			check[6] = '\0';
			if(!strcmp(check, "text/*")){
				strcpy(filename,"output.txt");
			}
			FILE* fptr;
			fptr = fopen(filename, "w");
			if(fork()==0){
				char *args[4];
				args[0] = "cp";
				args[1] = "out.txt";
				args[2] = filename;
				args[3] = NULL;
				execvp(args[0], args);
			}
			else{
				wait(NULL);
			}
			fclose(fptr);
			if(fork()==0){
				char *args[2];
				args[0] = "xdg-open";
				args[1] = filename;
				args[2] = NULL;
				execvp(args[0], args);
			}
			else{
				wait(NULL);
			}
			close(sockfd);
		}
		else if(command[0] == 'P' && command[1] == 'U' && command[2] == 'T'){
			memset(buf, '\0', sizeof(buf));
			strncpy(buf, command+4, strlen(command)-4);
			buf[strlen(buf)] = '\0';
			err = sscanf( buf, "%*[^/]%*[/]%[^/]", host);
			err = sscanf(buf, "%*[^/]%*[/]%*[^/]%[^: ]", url);
			err = sscanf(buf, "%*[^/]%*[/]%*[^/]%*[^:]%*[:]%[^ ]", port);
			err = sscanf(buf, "%*[^ ]%*[ ]%s", file);
			int p_no;
			if(strlen(port) == 0){
				p_no = 80;
			}
			else{
				p_no = atoi(port);
			}
			serv_addr.sin_family	= AF_INET;
			inet_aton(host, &serv_addr.sin_addr);
			serv_addr.sin_port	= htons(p_no);
			if ((connect(sockfd, (struct sockaddr *) &serv_addr,
								sizeof(serv_addr))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}
			http = PUT_req(host, url, port, file);
			printf("\n\nhttp: \n%s\n\n", http);
			int size = strlen(http);
			for(int i=0; i<=size; i+=50){
				for(int j=0; j<50; j++){buf[j] = '\0';}
				if(i == size){
					send(sockfd, buf, strlen(buf) + 1, 0);
					continue;
				}
				strncpy(buf, http + i, min(size - i, 50));
				if(size - i >= 50){
					send(sockfd, buf, 50, 0);
				}
				else{
					send(sockfd, buf, size-i, 0);
				}
			}
			FILE *fp;
			fp = fopen(file, "rb");
			char *file_content;
			file_content = (char *)malloc(SEND_BUFF_LIMIT * sizeof(char));
			int recv_siz = 0;
			while(1){
				if((recv_siz=fread(file_content, sizeof(char), SEND_BUFF_LIMIT, fp)) != SEND_BUFF_LIMIT){
					//printf("\n\n%s\n\n", file_content);
					send(sockfd, file_content, recv_siz, 0);
					break;
				}
				if (send(sockfd, file_content, SEND_BUFF_LIMIT, 0) < 0)
				{
					perror("send error : correct message not sent\n");
					exit(0);
				}
				//printf("\n\n%s\n\n", file_content);
			}
			char *recv_buff;
			recv_buff = (char*)malloc((RECV_BUFF_LIMIT+1)*sizeof(char));
			int recv_bytes;
			int cli_num=1;
			int curr_buff_size=0;
			unsigned int clilen;
			char *request;
			request = NULL;
			char file_name[100];
			fp = fopen("put.txt", "w");
            if(fp==NULL){
                perror("file error : unable to open file\n");
                exit(0);
            }
            char *protocol_version = NULL, *status_code = NULL, *status_msg = NULL;
            // char *body = NULL;
            int body_start = 0,flag=0,max_size;
            header *top;
            top = (header *)malloc(sizeof(header));
            top->next=NULL;
            // printf("hi uper\n");
            
            curr_buff_size = 0;
            while(1){       // iteratively receives the packets of information and dynamically expands the array
			                // if(check_if_complete(request, curr_buff_size)){
                //     break;
                // }
                //printf("hi \n");
                if(flag==1 && size>=max_size){
                    break;
                }
                if((recv_bytes = recv(sockfd, recv_buff, RECV_BUFF_LIMIT, 0))<0){
                    perror("receive error : correct message not received\n");
                    exit(0);
                }
				//printf("\n\n Flag starts here: %s\nEND", recv_buff);
				//fprintf(fp, "%s", recv_buff);
                if(flag==0){
                    size = check_body_start(recv_buff, &body_start, recv_bytes);
                    if(size>=0){
                        flag = 1;
						//printf("\n\n Flag starts here: %s\nEND", recv_buff);
                    }
                    transfer(recv_buff, &request, curr_buff_size, recv_bytes);
                    curr_buff_size += recv_bytes;
                    if(flag == 1){
                            //printf("hi %d \n",size );
                        max_size = process(&protocol_version, &status_code, &status_msg, top, request, curr_buff_size);
						//header_finder(top, "Content-Type");
                        if(max_size>= size){
                            //printf("hi %d %d\n",size, max_size);
                            int temp = recv_bytes - size;
							//printf("\nChecking: %s\nENDING\nSIZE: %d\n", recv_buff+temp, size);
                            int ret = fwrite(recv_buff+temp, sizeof(char), size, fp);
                        }
                        else{
                            int temp = recv_bytes - max_size;
                            fwrite(recv_buff+temp, sizeof(char), max_size, fp);
                        }
                    }
                }
                else{
                    size += recv_bytes;
                    if(size>=max_size){
                        recv_bytes = max_size - (size - recv_bytes);
                    }
                    recv_buff[recv_bytes] = '\0';
                    fprintf(fp, "%s", recv_buff);
                }
            }
			if(!strcmp(status_code, "200")){;}
			else if(!strcmp(status_code, "400")){
				printf("ERROR: HTTP 400 BAD REQUEST\n");
			}
			else if(!strcmp(status_code, "403")){
				printf("ERROR: HTTP 403 FORBIDDEN\n");
			}
			else if(!strcmp(status_code, "404")){
				printf("ERROR: HTTP 404 NOT FOUND\n");
			}
			else{
				printf("UNKNOWN ERROR: HTTP %s\n",status_code);
			}
			fclose(fp);
			printf("\n%s %s  %s\n",protocol_version, status_code, status_msg);
            header *it;
            it = top->next;
            while(it!=NULL){
                printf("%s: %s\n",it->key, it->value);
                it = it->next;
            }
			close(sockfd);
		}
		else if(!strcmp(command, "QUIT")){
			printf("Closing Program\n");
			break;
		}
		else{
			printf("Wrong Command. Send Again\n");
		}
	}
	return 0;
}

