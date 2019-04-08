#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> /* for hton * */
#include <sys/wait.h> /* sockets */
#include <sys/types.h> /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <netdb.h> /* gethostbyaddr */
#include <unistd.h> /* fork */
#include <ctype.h> /* toupper */
#include <signal.h> /* signal */
#include <sys/poll.h>
#include "intlist.h"
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "my_read_write.h"

using namespace std;

#define MSGSIZE 4096
#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

// GLOBALS
//define locks
pthread_mutex_t mtx;
pthread_mutex_t pagesmtx;
pthread_mutex_t bytesmtx;
pthread_cond_t cond_nonempty;

intlist* requests; //request buffer
char rootdir[512]; //root directory
int totalpages;
int totalbytes;

bool shutdownflg = false;

void* thread_function(void *){
    cout << "I am thread: " << pthread_self() << endl;
    int fd,headercount=0,filesize=0;
    char buffer[MSGSIZE] = "",copybuffer[MSGSIZE] = "",pagepath[MSGSIZE] = "",response[MSGSIZE] = "",freadbuf[MSGSIZE]="",intbuf[128]="";
    char *tok=NULL,*requestedpage = NULL,*temp=NULL,*rest;
    FILE *file;
    while(1){
        headercount = 0;
        //lock queue
        pthread_mutex_lock(&mtx);
        while(requests->totalitems<=0){//if there is something to remove
            pthread_cond_wait(&cond_nonempty,&mtx);//wait for the condition to be signalled
            if(shutdownflg){
                cout << "Exiting thread: " << pthread_self() << endl;
                pthread_mutex_unlock(&mtx);
                pthread_exit(NULL); 
            }
        }
        //cout << "totalitems = " << requests->totalitems << endl;
        fd = requests->remove();
        //unlock queue
        pthread_mutex_unlock(&mtx);
        //read from fd and reply
        if(myread(fd,buffer,MSGSIZE,"\r\n\r\n")){
            pagepath[0] = 0;
            cout << buffer << endl;
            if(strcmp(buffer+strlen(buffer)-2,"\r\n")==0){//if request is eligible
                //analyze requests
                strcpy(copybuffer,buffer);
                rest = copybuffer;
                tok = strtok_r(copybuffer," \t\n",&rest);
                while(tok!=NULL){//search for GET and Host fields
                    if(strcmp(tok,"GET")==0){
                        requestedpage = strtok_r(NULL," \t\n",&rest);
                        if(requestedpage != NULL){
                            headercount++;
                        }
                    }
                    else if(strcmp(tok,"Host:")==0){
                        if((strtok_r(NULL," \t\n",&rest)!=NULL)){
                            headercount++;
                        }
                    }
                    tok = strtok_r(NULL," \t\n",&rest);
                }
                if(headercount != 2){//request is invalid
                    cout << "Wrong request!" << endl;
                }
                else{//process request
                    memset(response,'\0',MSGSIZE);
                    cout << "requestedpage: " << requestedpage << endl;
                    strcat(pagepath,rootdir);
                    strcat(pagepath,requestedpage);
                    cout << "pagepath: " << pagepath << endl;
                    if((file = fopen(pagepath,"r")) == NULL){
                        if(errno == ENOENT){//file doesnt exist
                            cout << "file doesnt exist" << endl;
                            strcat(response,"HTTP/1.1 404 Not Found\nDate: Mon, 27 May 2018 12:28:53 GMT\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: ");
                            strcpy(freadbuf,"<html>Sorry dude, couldn't find this file.</html>");
                            sprintf(intbuf,"%d",(int)strlen(freadbuf));
                            pthread_mutex_lock(&bytesmtx);
                            totalbytes += (int)strlen(freadbuf);
                            pthread_mutex_unlock(&bytesmtx);
                            strcat(response,intbuf);
                            strcat(response,"\nContent-Type: text/html\nConnection: Closed\r\n\r\n");
                            strcat(response,freadbuf);
                            mywrite(fd,response,MSGSIZE);
                            //cout << response << endl;
                        }
                        else if(errno == EACCES){//no read rights
                            cout << "no read rights" << endl;
                            strcat(response,"HTTP/1.1 403 Forbidden\nDate: Mon, 27 May 2018 12:28:53 GMT\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: ");
                            strcpy(freadbuf,"<html>Trying to access this file but don't think I can make it.</html>");
                            sprintf(intbuf,"%d",(int)strlen(freadbuf));
                            pthread_mutex_lock(&bytesmtx);
                            totalbytes += (int)strlen(freadbuf);
                            pthread_mutex_unlock(&bytesmtx);
                            strcat(response,intbuf);
                            strcat(response,"\nContent-Type: text/html\nConnection: Closed\r\n\r\n");
                            strcat(response,freadbuf);
                            mywrite(fd,response,MSGSIZE);
                            //cout << response << endl;
                        }
                    }
                    else{//send response with file content
                        cout << "send response with file content" << endl;
                        memset(freadbuf,'\0',MSGSIZE);
                        strcat(response,"HTTP/1.1 200 OK\nDate: Mon, 27 May 2018 12:28:53 GMT\nServer: myhttpd/1.0.0 (Ubuntu64)\nContent-Length: ");
                        fseek (file , 0 , SEEK_END);
                        filesize = ftell(file);
                        rewind(file);
                        sprintf(intbuf,"%d",filesize);
                        pthread_mutex_lock(&bytesmtx);
                        totalbytes += filesize;
                        pthread_mutex_unlock(&bytesmtx);
                        strcat(response,intbuf);
                        strcat(response,"\nContent-Type: text/html\nConnection: Closed\r\n\r\n");
                        while(fread(freadbuf,1,MSGSIZE-strlen(response)-1,file)!=0){
                            strcat(response,freadbuf);
                            //cout << response << endl;
                            mywrite(fd,response,MSGSIZE);
                            memset(response,'\0',MSGSIZE);
                            memset(freadbuf,'\0',MSGSIZE);
                        }
                    }
                    memset(response,'\0',MSGSIZE);
                    memset(freadbuf,'\0',MSGSIZE);
                    pthread_mutex_lock(&pagesmtx);
                    totalpages += 1;
                    pthread_mutex_unlock(&pagesmtx);
                }
                while(1){//wait for client to read response and then close the socket
                    if(read(fd,response,MSGSIZE) == 0){
                        break;
                    }
                }
                close(fd);
            }
            memset(buffer,'\0',MSGSIZE);
        }
        
    }
}

//./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir
//./myhttpd -p 8080 -c 9090 -t 4 -d /home/alex/Desktop/project3/root_directory/
int main(int argc, char* argv[]){
    totalpages = 0;
    totalbytes = 0;
    time_t startTime,upTime;
    startTime = time(NULL);

    if(argc!=9){
        cout << "Not enough arguments" << endl;
        return 0;
    }

    //initialize mutex locks
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&pagesmtx, 0);
    pthread_mutex_init(&bytesmtx, 0);
    pthread_cond_init(&cond_nonempty, 0);

    //initialize variables of arguments
    int servport = atoi(argv[2]),cmdport=atoi(argv[4]),nothreads=atoi(argv[6]),servsock,newservsock,cmdsock,newcmdsock,err;
    strcpy(rootdir,argv[8]);
    cout << rootdir << endl;
    int enable = 1;

    //setup serving socket
    if ((servsock=socket(AF_INET,SOCK_STREAM,0)) < 0)
        perror("socket") ;
    struct sockaddr_in servserver,servclient;
    socklen_t servclientlen;
    servserver.sin_family = AF_INET;
    servserver.sin_addr.s_addr = htonl(INADDR_ANY);
    servserver.sin_port = htons(servport);
    if (setsockopt(servsock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (bind(servsock,(struct sockaddr*)&servserver,sizeof(servserver)) < 0)
        perror("bind");
    if (listen(servsock,100) < 0)
        perror("listen");
    cout << "Listening for connections to port " << servport << endl;

    //setup command socket
    if ((cmdsock=socket(AF_INET,SOCK_STREAM,0)) < 0)
        perror("socket");
    struct sockaddr_in cmdserver,cmdclient;
    socklen_t cmdclientlen;
    cmdserver.sin_family = AF_INET;
    cmdserver.sin_addr.s_addr = htonl(INADDR_ANY);
    cmdserver.sin_port = htons(cmdport);
    if (setsockopt(cmdsock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    if (bind(cmdsock,(struct sockaddr*)&cmdserver,sizeof(cmdserver)) < 0)
        perror("bind");
    if (listen(cmdsock,100) < 0)
        perror("listen");
    cout << "Listening for connections to port " << cmdport << endl;

    char buffer[MSGSIZE]="";

    struct pollfd pollfds[2];
	int ret;

	/* watch stdin for input */
	pollfds[0].fd = servsock;
	pollfds[0].events = POLLIN;

	/* watch stdout for ability to write */
	pollfds[1].fd = cmdsock;
	pollfds[1].events = POLLIN;

    //initialize queue for requests
    requests = new intlist();

    //initialize thread_ids
    pthread_t *threads;
    threads = (pthread_t*)malloc(nothreads*sizeof(pthread_t));
    for(int i=0;i<nothreads;i++){
        if(err = pthread_create(&threads[i],NULL,thread_function,NULL)){
            perror2("pthread_create", err);
            exit(1);
        }
    }

    char responsebuf[MSGSIZE]="";
    while(1){
        ret = poll(pollfds, 2, -1);
        if (ret == -1) {
            perror ("poll");
            return 1;
        }
        //serving socket
        if (pollfds[0].revents & POLLIN){
            if ((newservsock = accept(servsock,(struct sockaddr*)&servclient,&servclientlen )) < 0)
                perror("accept");
            cout << "Accepted serving port connection" << endl;
            //lock queue
            pthread_mutex_lock(&mtx);
            requests->insert(newservsock);
            //unlock queue
            pthread_mutex_unlock(&mtx);
            //notify with cond variable that list isnt empty
            pthread_cond_signal(&cond_nonempty);
            cout << "inserted: " << newservsock << endl;
        }
        //command socket
        if (pollfds[1].revents & POLLIN){
            if ((newcmdsock = accept(cmdsock,(struct sockaddr*)&cmdclient,&cmdclientlen)) < 0)
                perror("accept");
            cout << "Accepted command port connection" << endl;
            if(myread(newcmdsock,buffer,MSGSIZE,"\r\n")!=-1){
                //cout << buffer << endl;
                if(strcmp(buffer,"STATS\r\n")==0){
                    upTime = time(NULL) - startTime;
                    //cout << "Server up for " << upTime << " seconds, served " << totalpages << " pages, " << totalbytes << " bytes"<< endl;
                    sprintf(responsebuf,"Server up for %d seconds, served %d pages, %d bytes\n",(int)upTime,totalpages,totalbytes);
                    mywrite(newcmdsock,responsebuf,MSGSIZE);
                }
                else if(strcmp(buffer,"SHUTDOWN\r\n")==0){
                    cout << "Free memory and shutdown" << endl;
                    pthread_mutex_lock(&mtx);
                    shutdownflg = true;
                    pthread_mutex_unlock(&mtx);
                    pthread_cond_broadcast(&cond_nonempty);
                    for(int i=0;i<nothreads;i++){
                        if(err = pthread_join(threads[i],NULL)){
                            perror2("pthread_join", err);
                            exit(1);
                        }
                    }
                    free(threads);
                    requests->freelist();
                    delete requests;
                    cout << "myhttpd is exiting" << endl;
                    close(servsock);
                    close(cmdsock);
                    close(newcmdsock);
                    return 0;
                }
                else{
                    cout << "Wrong command!" << endl;
                }
                memset(buffer,'\0',MSGSIZE);
            }
            close(newcmdsock);
        }
    }
    return 0;
}