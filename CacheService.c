#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_BYTES 4096    
#define MAX_CLIENTS 400   
#define MAX_SIZE 200*(1<<20)     
#define MAX_ELEMENT_SIZE 10*(1<<20)    


struct cache_element{
    char* data;         
    int len;          
    char* url;        
	time_t lru_time_track;   
    cache_element* next;    
};
typedef struct cache_element cache_element;


cache_element* find(char* url);
int add_cache_element(char* data,int size,char* url);
void remove_cache_element();


int port_number = 8080;				
int proxy_socketId;				
pthread_t tid[MAX_CLIENTS];     
sem_t seamaphore;	  
pthread_mutex_t lock;
cache_element* head;
int cache_size;

void error_handler(char* s){
    perror(s);
    exit(1);
}

void *thread_fn(void *socketNew){
    sem_wait(&seamaphore);
    int p;
    sem_getvalue(&seamaphore,&p);
    printf("semaphore value:%d\n",p);

    int *t = (int*)  socketNew;
    int socket = *t;
    int byte_send_client , len;
    
    char* buffer = (char*)calloc(MAX_BYTES,sizeof(char));
    bzero(buffer, MAX_BYTES);
    byte_send_client = recv(socket, buffer, MAX_BYTES, 0);

    while(byte_send_client >  0 ){
        len = strlen(buffer);
        if(strstr(buffer , "\r\n\r\n") == NULL){
            byte_send_client = recv(socket, buffer + len, MAX_BYTES - len, 0);
        }else{
            break;
        }
    }

    char *tempReq = (char*)malloc(strlen(buffer)*sizeof(char)+1);
    for(int i = 0 ; i < strlen(buffer) ; i++){
        tempReq[i] = buffer[i];
    } 

    struct chache_element* temp = find(tempReq);
    if(temp!= NULL){
        int size = temp->len/sizeof(char);
        int pos = 0 ; 
        char response[MAX_BYTES];
        while(pos<size){
            bzero((char*)&response,MAX_BYTES);
            for(int i = 0 ; i < MAX_BYTES ; i++){
                response[i] = temp->data[pos];
                pos++;
            }
            send(socket,response,MAX_BYTES,0);
        }
        printf("Data retrived from the Cache\n\n");
        printf("%s\n\n",response);
    }else if(byte_send_client > 0){
        len = strlen(buffer);
        ParsedRequest *request = ParsedRequest_create();
        if(ParsedRequest_parse(request, buffer, len) < 0){
            printf("Error in parsing the request\n");
            free(tempReq);
            return NULL;
        }else{
             bzero((char*)&buffer , MAX_BYTES);
             if(!strcmp(request->method, "GET")){
                if(request->host && request->path && checkHTTPversion(request-<version)==1){
                    
                }
             }
        }


    }
}


int main(int argc , char* argv[]){
    int client_socketId , client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&seamaphore,0,MAX_CLIENTS);
    pthread_mutex_init(&lock,NULL);

    if(argv == 2){
        port_number = atoi(argv[1]);
    }
    else{
        printf("Too few arguments\n");
        exit(1);
    }

    printf("Setting Proxy Server Port : %d\n",port_number);
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_socketId < 0) error_handler("Failed to create socket.\n");

    int reuse = 1 ;
    if(setsockopt(proxy_socketId , SOL_SOCKET , SO_REUSEADDR , (const char*)&reuse , sizeof(reuse)) < 0)  error_handler("setsockopt(SO_REUSEADDR) failed\n");

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(proxy_socketId , (struct sockaddr*)&server_addr , sizeof(server_addr) < 0)) error_handler("Port is not available");

    int listen_status = listen(proxy_socketId , MAX_CLIENTS);
    if(listen_status < 0) error_handler("Error while Listening !\n");

    int i = 0;
    int connected_socketId[MAX_CLIENTS];

    while(1){
        bzero((char*)&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketId = accept(proxy_socketId, (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
        if(client_socketId < 0){
            error_handler("Error in Accepting connection !\n");
        }
        else{
            connected_socketId[i] = client_socketId;
        }
    
    struct sockaddr_in * client_ptr = (struct sockaddr_in*)&client_addr;
    struct in_addr ip_addr = client_ptr->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);

    printf("Client is connected with port number: %d and ip address: %s \n", ntohs(client_addr.sin_port), str);

    pthread_create(&tid[i], NULL, thread_fn, (void*)&connected_socketId[i]);
    i++;    
    }
    close(proxy_socketId);
    return 0;
}


