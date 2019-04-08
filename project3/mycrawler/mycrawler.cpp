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
#include "strlist.h"
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "../myhttpd/my_read_write.h"

using namespace std;

#define MSGSIZE 4096
#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

extern int errno;
#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"
#define PERMS 0666

// GLOBALS
//define locks
pthread_mutex_t mtx; //url queue mutex
pthread_mutex_t pagesmtx;
pthread_mutex_t bytesmtx;
pthread_cond_t cond_nonempty;

char *save_dir;
strlist *urls; //urls to be searched
strlist *searchedUrls; //already searched urls
int totalpages;
int totalbytes;
int threadswaiting;
struct sockaddr_in servserver;

bool shutdownflg = false;

void* thread_function(void *argp){
    int fd,filesize,totalread=0,readcount=0,headersize,k=0,l=0,enable=1;
    char *url=NULL,request[MSGSIZE]="",response[MSGSIZE]="",copyresponse[MSGSIZE]="",*tok,*endofheader,buffer[MSGSIZE]="",link[MSGSIZE]="";
    char filename[MSGSIZE]="",copyurl[MSGSIZE]="",temp[MSGSIZE]="";
    char *writebuf,*filebuf,*rest;
    
    cout << "I am thread: " << pthread_self() << endl;
    while(1){
        pthread_mutex_lock(&mtx);
        while(urls->totalitems<=0){//if there is something to remove
            threadswaiting++;
            //cout << "thread: " << pthread_self() << " waiting" << endl;
            pthread_cond_wait(&cond_nonempty,&mtx);//wait for the condition to be signalled
            //cout << "thread: " << pthread_self() << " waking up" << endl;
            threadswaiting--;
            if(shutdownflg){
                cout << "Exiting thread: " << pthread_self() << endl;
                pthread_mutex_unlock(&mtx);
                pthread_exit(NULL); 
            }
        }
        url = urls->remove();
        if(searchedUrls->search(url) == 0){//insert starting url to already searched list
            searchedUrls->insert(url);
        }
        pthread_mutex_unlock(&mtx);
        readcount = 0;
        memset(filename,'\0',MSGSIZE);
        memset(temp,'\0',MSGSIZE);
        //prepare http get request
        sprintf(request,"GET %s HTTP/1.1\nUser-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\nHost: www.tutorialspoint.com\nAccept-Language: en-us\nAccept-Encoding: gzip, deflate\nConnection: Keep-Alive\r\n\r\n",url);
        if ((fd=socket(AF_INET,SOCK_STREAM,0)) < 0)
            perror("socket");
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            perror("setsockopt(SO_REUSEADDR) failed");
        //initiate connection
        if (connect(fd, (struct sockaddr*)&servserver, sizeof(servserver)) < 0)
            perror("connect");
        //write request
        mywrite(fd,request,MSGSIZE);
        //read responses
        memset(response,'\0',MSGSIZE);
        readcount += myread(fd,response,MSGSIZE,NULL);//assume that the http header fits in the first MSGSIZE bytes
        strcpy(copyresponse,response);
        rest = copyresponse;
        tok = strtok_r(copyresponse," \n",&rest);
        while(tok!=NULL){//get content length from http response
            if(strcmp(tok,"Content-Length:")==0){
                if((tok = strtok_r(NULL," \n",&rest))!=NULL){
                    filesize = atoi(tok);
                }
            }
            tok = strtok_r(NULL," \n",&rest);
        }
        if((endofheader = strstr(response,"\r\n\r\n"))!=NULL){
            headersize = strlen(response) - (strlen(endofheader)-4);
        }
        //cout << "headersize: " << headersize << endl;
        totalread = headersize+filesize;
        mkdir(save_dir,0755);
        strcpy(copyurl,url);
        rest = copyurl;
        tok = strtok_r(copyurl,"/",&rest);
        strcat(temp,save_dir);
        strcat(temp,"/");
        strcat(temp,tok);
        mkdir(temp,0755);
        strcat(filename,save_dir);
        strcat(filename,url);
        free(url);
        //opening file to write
        FILE *file = fopen(filename,"a");
        if(file == NULL)
            perror("Error opening file.");
        
        writebuf = (char*)malloc(totalread*sizeof(char));
        filebuf = (char*)malloc(totalread*sizeof(char));
        memset(writebuf,'\0',totalread);
        memset(filebuf,'\0',totalread);

        strcat(filebuf,response);
        int readbytes;
        //read http response
        while(readcount < totalread){
            memset(response,'\0',MSGSIZE);
            if((readbytes = myread(fd,response,MSGSIZE,NULL)) == 0){
                break;
            }
            strcat(filebuf,response);
            readcount += readbytes;
            //cout << readcount << " " << totalread << endl;
        }
        k=0;
        int i=headersize;
        //analyze the html page
        while(i<(int)strlen(filebuf)){
            if(filebuf[i] == '<'){
                if(filebuf[i+1]=='a'){//add link to url queue
                    memset(link,'\0',MSGSIZE);
                    i+=2;
                    l=0;
                    while(i<(int)strlen(filebuf)){
                        if(filebuf[i]=='='){
                            i++;
                            while(i<(int)strlen(filebuf) && filebuf[i]!='>'){
                                link[l] = filebuf[i];
                                l++;
                                i++;
                            }
                            //add link to queue
                            pthread_mutex_lock(&mtx);
                            if(searchedUrls->search(link) == 0){//url hasn't been requested before
                                searchedUrls->insert(link);
                                urls->insert(link);
                            }
                            pthread_mutex_unlock(&mtx);
                            pthread_cond_signal(&cond_nonempty);
                            break;
                        }
                        i++;
                    }
                }
                else{
                    while(i<(int)strlen(filebuf) && filebuf[i]!='>'){
                        i++;
                    }
                }
            }
            else{
                writebuf[k] = filebuf[i];
                k++;
            }
            i++;
        }

        //write text to html file
        fwrite(writebuf,sizeof(char),(int)strlen(writebuf),file);

        //free memory
        free(writebuf);
        free(filebuf);
        fclose(file);
        close(fd);
    }
}

//./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL
//./mycrawler -h localhost -p 8080 -c 9091 -t 1 -d save_dir /site0/page0_2076.html
int main(int argc, char* argv[]){
    if(argc!=12){
        cout << "Not enough arguments" << endl;
        return 0;
    }
    time_t startTime,upTime;
    startTime = time(NULL);

    char *host = argv[2],*starting_URL = argv[11],buffer[MSGSIZE]="",copybuffer[MSGSIZE]="",responsebuf[MSGSIZE]="",*tok=NULL,*rest;
    int hostport=atoi(argv[4]),cmdport=atoi(argv[6]),nothreads=atoi(argv[8]),cmdsock,newcmdsock,err;
    threadswaiting=0;
    save_dir = argv[10];
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&pagesmtx, 0);
    pthread_mutex_init(&bytesmtx, 0);
    pthread_cond_init(&cond_nonempty, 0);
    
    //setup connect to host socket
    int enable = 1;
    struct hostent *rem;
    if ((rem = gethostbyname(host)) == NULL) {
        herror("gethostbyname"); exit(1);
    }

    servserver.sin_family = AF_INET;
    memcpy(&servserver.sin_addr, rem->h_addr, rem->h_length);
    servserver.sin_port = htons(hostport);
    
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

    //initialize queue
    urls = new strlist();
    searchedUrls = new strlist();
    //insert starting url to queue
    urls->insert(starting_URL);

    //initialize thread_ids
    pthread_t *threads;
    threads = (pthread_t*)malloc(nothreads*sizeof(pthread_t));
    cout << nothreads << endl;
    for(int i=0;i<nothreads;i++){
        if(err = pthread_create(&threads[i],NULL,thread_function,NULL)){
            perror2("pthread_create", err);
        }
    }
    //make pipes for communication with jobExecutor
    if ( (mkfifo(FIFO1, PERMS) < 0) && (errno != EEXIST) ) {
        perror("can't create fifo");
    }
    if ( (mkfifo(FIFO2, PERMS) < 0) && (errno != EEXIST) ) {
        perror("can't create fifo");
    }
    int writefd,readfd,pid;
    bool firsttime=true;
    while(1){
        if ((newcmdsock = accept(cmdsock,(struct sockaddr*)&cmdclient,&cmdclientlen)) < 0)
            perror("accept");
        cout << "Accepted command port connection" << endl;
        if(myread(newcmdsock,buffer,MSGSIZE,"\r\n")){
            cout << buffer << endl;
            strcpy(copybuffer,buffer);
            rest = buffer;
            tok = strtok_r(buffer," \r\n",&rest);
            if(strcmp(tok,"STATS")==0){
                upTime = time(NULL) - startTime;
                //cout << "Server up for " << upTime << " seconds, served " << totalpages << " pages, " << totalbytes << " bytes"<< endl;
                sprintf(responsebuf,"Server up for %d seconds, served %d pages, %d bytes\n",(int)upTime,totalpages,totalbytes);
                mywrite(newcmdsock,responsebuf,strlen(responsebuf));
            }
            else if(strcmp(tok,"SEARCH")==0){
                pthread_mutex_lock(&mtx);
                if(threadswaiting == nothreads && urls->totalitems<=0){//crawling has ended
                    pthread_mutex_unlock(&mtx);
                    if(firsttime){//initialize jobExecutor
                        firsttime = false;
                        //creating docfile
                        FILE *docfile=NULL;
                        if((docfile = fopen("./docfile","a"))==NULL)
                            perror("fopen");
                        DIR *direct;
                        struct dirent *ent;
                        if ((direct = opendir(save_dir)) != NULL) {
                            /* print all the files and directories within directory */
                            while ((ent = readdir (direct)) != NULL) {
                                if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0){
                                    continue;
                                }
                                char filepath[512] = "";
                                strcat(filepath,save_dir);
                                strcat(filepath,"/");
                                strcat(filepath,ent->d_name);
                                strcat(filepath,"\n");
                                fwrite (filepath , sizeof(char), strlen(filepath), docfile);
                            }
                            fclose(docfile);
                            closedir(direct);
                        }
                        //Initializing jobExecutor
                        if((pid=fork())==-1)
                            perror("fork");
                        else if(pid == 0){//child
                            close(newcmdsock);
                            if(execl("./jobExecutor","jobExecutor","-d","./docfile","-w","10",NULL) == -1)
                                perror("execl");
                            exit(1);
                        }
                        while ( (writefd = open(FIFO1, O_WRONLY)) < 0){
                            //perror("client: can't open write fifo \n");
                        }
                        while ( (readfd = open(FIFO2, O_RDONLY)) < 0){
                            //perror("client: can't open read fifo \n");
                        }
                    }
                    //do the searching
                    write(writefd,copybuffer,MSGSIZE);
                    while(1){
                        memset(responsebuf,'\0',MSGSIZE);
                        read(readfd,responsebuf,MSGSIZE);
                        //cout << "responsebuf " << responsebuf << endl;
                        if(strcmp(responsebuf,"ENDOFSEARCH")==0){
                            //cout << responsebuf << endl;
                            break;
                        }
                        else{
                            mywrite(newcmdsock,responsebuf,strlen(responsebuf));
                        }
                    }
                }
                else{//crawling in progress
                    cout << "threads waiting = " << threadswaiting << " urls->totalitems = " << urls->totalitems << endl;
                    pthread_mutex_unlock(&mtx);
                    sprintf(responsebuf,"Crawling in-progress\n");
                    mywrite(newcmdsock,responsebuf,strlen(responsebuf));
                }
            }
            else if(strcmp(tok,"SHUTDOWN")==0){
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
                urls->freelist();
                delete urls;
                searchedUrls->freelist();
                delete searchedUrls;
                cout << "mycrawler is exiting" << endl;
                close(newcmdsock);
                close(cmdsock);
                return 0;
            }
            else{
                cout << "Wrong command!" << endl;
            }
            memset(buffer,'\0',MSGSIZE);
        }
        //cout << "closing socket" << endl;
        close(newcmdsock);
    }
}