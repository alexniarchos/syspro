#include <iostream>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include "signal.h"
#include "dirent.h"
#include "trie.h"
#include "commands.h"
#include "globals.h"
#include "time.h"

extern int errno;

#define FIFO1 "/tmp/fifo1"
#define FIFO2 "/tmp/fifo2"
#define PERMS 0666

using namespace std;

//GLOBAL VARIABLES
int linecounter = 0;
int numWorkers = 0;
pid_t *workers = NULL;

void childcode(int id){
    //Create trie
        trienode *root = new trienode();
        int readfd,writefd,noline = 0;
        char pipename[128] = "",pipename2[128] = "",buf[128] = "";
        sprintf(buf,"%d",id);
        strcat(pipename,"tmp/pipeSRCW_");
        strcat(pipename,buf);
        strcat(pipename2,"tmp/pipeSWCR_");
        strcat(pipename2,buf);
        //cout << "client pipename = " << pipename << endl;
        //cout << "client pipename2 = " << pipename2 << endl;
        while ( (writefd = open(pipename2, O_WRONLY)) < 0){
            //perror("client: can't open write fifo \n");
        }
        while ( (readfd = open(pipename, O_RDONLY)) < 0){
            //perror("client: can't open read fifo \n");
        }
        //cout << "my parent is " << getppid() << endl;
        char buffer[MAXBUFF] = "";
        char *tok;
        int counter=0; 
        char *received = NULL;
        received = receive(readfd);
        //cout << "RECEIVED MAP STRING: " << received << endl;
        //build the map
        tok = strtok(received," \t\n");
        int nofiles = atoi(tok);
        char **files[nofiles];
        int nolines[nofiles] = {0};
        for(int i=0;i<nofiles;i++){
            tok = strtok(NULL," \t\n");
            files[i] = (char**)malloc(atoi(tok)*sizeof(char*));
            nolines[i] = atoi(tok);
        }
        free(received);
        char *filepath = NULL;
        for(int i=0;i<nofiles;i++){
            filepath = receive(readfd);//read filepath
            for(int j=0;j<nolines[i];j++){
                if((received = receive(readfd)) != NULL){
                    files[i][j] = (char*)malloc((strlen(received)+1) * (sizeof(char)));
                    strcpy(files[i][j],received);
                    tok = strtok(received," \t\n");
                    while(tok!=NULL){
                        //cout << "tok = " << tok << "\tnoline: " << j << endl;
                        insertTrie(root,tok,filepath,i,j);
                        tok = strtok(NULL," \t\n");
                    }
                }
                else{
                    cout << "Wrong line counting of file: " << i << endl;
                }
                free(received);
            }
            free(filepath);
        }
        
        //wait for commands
        char *cmd = NULL,*args;
        size_t len = 0;
        char copycmd[128];
        int count=0;
        while(1){
            if((cmd = receive(readfd)) != NULL){
                //cout << cmd << endl;
                strcpy(copycmd,cmd);
                tok = strtok(cmd," \t\r\n");
                if(strcmp(tok,"SEARCH")==0){
                    double deadline;
                    //cout << "executing /search" << endl;
                    char *queries[QUERIES] = {NULL};
                    int i = 0;
                    tok = strtok(NULL," \t\r\n");
                    while(tok!=NULL){
                        if(strcmp(tok,"-d")==0){
                            tok = strtok(NULL," \t\r\n");
                            deadline = atoi(tok);
                            break;
                        }
                        queries[i++] = tok;
                        tok = strtok(NULL," \t\r\n");
                    }
                    count += search(files,root,writefd,queries);
                }
                else if(strcmp(tok,"/maxcount")==0){
                    //cout << "executing /maxcount" << endl;
                    tok = strtok(NULL," \t\r\n");
                    if(tok!=NULL){
                        //cout << "keyword: " << tok << endl;
                        maxcount(root,writefd,tok);
                    }
                    else{
                        cout << "wrong arguments" << endl;
                    }
                }
                else if(strcmp(tok,"/mincount")==0){
                    //cout << "executing /mincount" << endl;
                    tok = strtok(NULL," \t\r\n");
                    if(tok!=NULL){
                        //cout << "keyword: " << tok << endl;
                        mincount(root,writefd,tok);
                    }
                    else{
                        cout << "wrong arguments" << endl;
                    }
                }
                else if(strcmp(tok,"/wc")==0){
                    //cout << "executing /wc" << endl;
                    wc(writefd,files,nofiles,nolines);
                }
                else if(strcmp(tok,"/exit")==0){
                    //cout << "Exiting commands" << endl;
                    char numbuff[256];
                    sprintf(numbuff,"%d",count);
                    send(writefd,numbuff);
                    close(writefd);
                    close(readfd);
                    //Free MAP
                    for(int i=0;i<nofiles;i++){
                        for(int j=0;j<nolines[i];j++){
                            free(files[i][j]);
                        }
                        free(files[i]);
                    }
                    freeTrie(root);
                    free(cmd);
                    return;
                }
                else{
                    cout << "Wrong command!!!" << endl;
                }
            }
            free(cmd);
            cmd = NULL;
        }
}

void catchinterrupt(int signo){
    int child_status;
    pid_t pid;
    while ((pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
        //safe_printf("Received signal %d from process %d\n",signo, pid);
        for(int i=0;i<numWorkers;i++){
            if(workers[i] == pid){
                workers[i] = -1;
                //safe_printf("killed worker[%d] with pid = %d\n",i,pid);
            }
        }
    }
    return;
}

int main(int argc, char* argv[]){
    static struct sigaction act ;
    act.sa_handler = catchinterrupt;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask),SIGCHLD);
    sigaction(SIGCHLD , &act , NULL );
    int argnum;
    //check arguments
    if(argc != 5){
        cout << "wrong arguments" << endl;
        return 0;
    }
    if(strcmp(argv[1],"-d") == 0){// ./jobExecutor -d docfile -w numWorkers
        cout << "Filename: " << argv[2] << endl; 
        argnum = 2;
        if(strcmp(argv[3],"-w") == 0){
            numWorkers = atoi(argv[4]);
            cout << "numWorkers = " << numWorkers << endl;
        }
        else{
            cout << "Wrong argument: expected -w" << endl;
        }
    }
    else if(strcmp(argv[1],"-w") == 0){// ./minisearch -k (5) -i docfile
        numWorkers = atoi(argv[2]);
        if(strcmp(argv[3],"-d") == 0){
            cout << "Filename: " << argv[4] << endl;
            argnum = 4;
        }
        else{
            cout << "Wrong argument: expected -d" << endl;
            return 0;
        }
    }
    else{
        cout << "Wrong argument: expected -d or -w" << endl;
        return 0;
    }
    
    //Document counting
    char c,last;
    FILE *file;
    file = fopen (argv[argnum],"r");
    if (file != NULL)
    {
        c=fgetc(file);
        while(1){
            if((c == '\n' || c == EOF) && last != '\n'){
                linecounter++;
            }
            if(c == EOF){
                break;
            }
            last = c;
            c=fgetc(file);
        }
        cout << "linecounter: " << linecounter << endl;
        fclose (file);
    }
    else{
        perror("fopen");
        return 0;
    }

    //if there are more workers than directory paths then workers are decreased to match the number of paths
    if(numWorkers > linecounter){
        numWorkers = linecounter;
    }

    workers = (pid_t*)malloc(numWorkers*sizeof(pid_t));
    int workerload[numWorkers] = {};
    size_t len = 0;

    int childpid,id;
    for(int i=0;i<numWorkers;i++){
        if ( (childpid = fork()) == 0 ){//only parent continues generating childrens
            id = i;
            break;
        }
        workers[i] = childpid;
    }

    if(childpid == 0){//child code
        childcode(id);
    }
    else{//parent code
        //open file to read directories
        file = fopen (argv[argnum],"r");
        //Create the named FIFOs, then open them -- one for reading and one for writing.
        char pipename[128] = "",pipename2[128] = "",buf[128] = "";
        int readfd[numWorkers],writefd[numWorkers];
        for(int i=0;i<numWorkers;i++){
            sprintf(buf,"%d",i);
            strcat(pipename,"tmp/pipeSRCW_");
            strcat(pipename,buf);
            //cout << "pipename = " << pipename << endl;
            if ( (mkfifo(pipename, PERMS) < 0) && (errno != EEXIST) ) {
                perror("can't create fifo");
            }
            strcat(pipename2,"tmp/pipeSWCR_");
            strcat(pipename2,buf);
            //cout << "pipename2 = " << pipename2 << endl;
            if ( (mkfifo(pipename2, PERMS) < 0) && (errno != EEXIST) ) {
                perror("can't create fifo");
            }
            while ( (readfd[i] = open(pipename2, O_RDONLY)) < 0){
                //perror("server: can't open read fifo \n");
            }
            while ( (writefd[i] = open(pipename, O_WRONLY)) < 0){
                //perror("server: can't open write fifo \n");
            }
            pipename[0] = 0;
            pipename2[0] = 0;
        }
        for(int i=0;i<numWorkers;i++){
            cout << "workers[" << i << "] = " << workers[i] << endl;
        }
        for(int i=0;i<numWorkers;i++){
            workerload[i] = linecounter/numWorkers;
        }
        for(int i=0;i<linecounter % numWorkers;i++){
            workerload[i] += 1;
        }
        //directories get equally distributed to workers
        char *dir = NULL,buffer[MAXBUFF]="";
        int filecounter = 0;
        char filelines[512] = "";
        char infolines[numWorkers][512];
        FILE *tempfile;
        int biggestline=-1;
        for(int i=0;i<numWorkers;i++){
            filecounter = 0;
            char numbuffer[128] = "";
            char sendbuffer[256] = "";
            //cout << "workerload["<<i<<"]= "<<workerload[i]<<endl;
            filelines[0] = 0;
            for(int k=0;k<workerload[i];k++){
                if(getline(&dir,&len,file)!=-1){
                    if(dir[strlen(dir)-1] == '\n')
                        dir[strlen(dir)-1]='\0';
                    DIR *direct;
                    struct dirent *ent;
                    if ((direct = opendir (dir)) != NULL) {
                        /* print all the files and directories within directory */
                        while ((ent = readdir (direct)) != NULL) {
                            int linecounter = 0;
                            if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0){
                                continue;
                            }
                            char filepath[256] = "";
                            strcat(filepath,dir);
                            strcat(filepath,"/");
                            strcat(filepath,ent->d_name);
                            tempfile = fopen(filepath,"r");
                            if(tempfile == NULL){
                                perror("");
                                continue;
                            }
                            while(fgets(buffer,MAXBUFF,tempfile) != NULL){
                                if(biggestline < (int)strlen(buffer)){
                                    biggestline = strlen(buffer);
                                }
                                linecounter++;
                            }
                            sprintf(numbuffer,"%d",linecounter);
                            strcat(filelines,numbuffer);
                            strcat(filelines," ");
                            filecounter++;
                            fclose(tempfile);
                        }
                        closedir (direct);
                    } 
                    else{
                        /* could not open directory */
                        perror ("");
                        return EXIT_FAILURE;
                    }
                }
                free(dir);
                dir = NULL;
            }
            //cout << "filecounter= " << filecounter << endl;
            sprintf(numbuffer,"%d",filecounter);
            strcat(sendbuffer,numbuffer);
            strcat(sendbuffer," ");
            strcat(sendbuffer,filelines);
            strcpy(infolines[i],sendbuffer);
            //cout << "Sending to worker: " << i << " string: " << sendbuffer << endl;
            send(writefd[i],sendbuffer);
        }
        //cout << "Biggest line is: " << biggestline << " and MAXBUFF: " << MAXBUFF << endl;
        rewind(file);

        for(int i=0;i<numWorkers;i++){
            for(int j=0;j<workerload[i];j++){
                if(getline(&dir,&len,file)!=-1){
                    //send line to the child through the pipe
                    //cout << "writefd = " << writefd[i] << endl;
                    if(dir[strlen(dir)-1] == '\n')
                        dir[strlen(dir)-1]='\0';
                    //open directory and read files
                    DIR *direct;
                    struct dirent *ent;
                    if ((direct = opendir (dir)) != NULL) {
                        /* print all the files and directories within directory */
                        while ((ent = readdir (direct)) != NULL) {
                            //printf ("%s\n", ent->d_name);
                            //cout << "OPENING: " << "<<" << ent->d_name << ">>" << endl;
                            if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0){
                                continue;
                            }
                            //open dir, send text to worker
                            char filepath[256] = "";
                            strcat(filepath,dir);
                            strcat(filepath,"/");
                            strcat(filepath,ent->d_name);
                            tempfile = fopen(filepath,"r");
                            if(tempfile == NULL){
                                perror("");
                                continue;
                            }
                            send(writefd[i],filepath);
                            //cout << "Sending to worker: " << i << " string: " << filepath << endl;
                            while(fgets(buffer,MAXBUFF,tempfile) != NULL){
                                //cout << "Sending to worker: " << i << " string: " << buffer << endl;
                                send(writefd[i],buffer);
                            }
                            fclose(tempfile);
                        }
                        closedir (direct);
                    } 
                    else{
                        /* could not open directory */
                        perror ("");
                        return EXIT_FAILURE;
                    }
                }
                free(dir);
                dir = NULL;
            }
        }
        //wait for commands
        char cmd[MAXBUFF]="",*args;
        size_t len = 0;
        char copycmd[128];
        char *received = NULL;
        char *tok;
        int wfd,rfd;
        while ( (rfd = open(FIFO1, O_RDONLY)) < 0){
            //perror("client: can't open write fifo \n");
        }
        while ( (wfd = open(FIFO2, O_WRONLY)) < 0){
            //perror("client: can't open read fifo \n");
        }
        while(1){
            //cout << "syspro:/>";
            //cout << endl;
            // for(int i=0;i<numWorkers;i++){
            //     cout << "workers[" << i << "] = " << workers[i] << endl;
            // }
            //clearerr(stdin);
            //if((getline(&cmd, &len, stdin) != -1) && (strcmp(cmd,"\n")!=0)){
            if(read(rfd,cmd,MAXBUFF)==-1)
                perror("read");
            else if(strcmp(cmd,"\n")!=0){
                //check for killed workers and revive them
                for(int i=0;i<numWorkers;i++){
                    if(workers[i] == -1){
                        close(readfd[i]);
                        close(writefd[i]);
                        sprintf(buf,"%d",i);
                        strcat(pipename,"tmp/pipeSRCW_");
                        strcat(pipename,buf);
                        //cout << "pipename = " << pipename << endl;
                        strcat(pipename2,"tmp/pipeSWCR_");
                        strcat(pipename2,buf);
                        //cout << "pipename2 = " << pipename2 << endl;
                        unlink(pipename);
                        unlink(pipename2);
                        if ( (mkfifo(pipename, PERMS) < 0) && (errno != EEXIST) ) {
                            perror("can't create fifo");
                        }
                        if ( (mkfifo(pipename2, PERMS) < 0) && (errno != EEXIST) ) {
                            perror("can't create fifo");
                        }
                        if ( (childpid = fork()) == 0 ){//only parent continues generating childrens
                            childcode(i);
                            cout << "---Exiting PID = " << getpid() << endl;
                            return 0;
                        }
                        workers[i] = childpid;
                        while ( (readfd[i] = open(pipename2, O_RDONLY)) < 0){
                            //perror("server: can't open read fifo \n");
                        }
                        while ( (writefd[i] = open(pipename, O_WRONLY)) < 0){
                            //perror("server: can't open write fifo \n");
                        }
                        pipename[0] = 0;
                        pipename2[0] = 0;
                        rewind(file);
                        for(int k=0;k<numWorkers;k++){
                            for(int j=0;j<workerload[k];j++){
                                if(getline(&dir,&len,file)!=-1){
                                    if(i!=k)
                                        continue;
                                    send(writefd[i],infolines[i]);
                                    //send line to the child through the pipe
                                    //cout << "writefd = " << writefd[i] << endl;
                                    if(dir[strlen(dir)-1] == '\n')
                                        dir[strlen(dir)-1]='\0';
                                    //open directory and read files
                                    DIR *direct;
                                    struct dirent *ent;
                                    if ((direct = opendir (dir)) != NULL) {
                                        /* print all the files and directories within directory */
                                        while ((ent = readdir (direct)) != NULL) {
                                            //printf ("%s\n", ent->d_name);
                                            //cout << "OPENING: " << "<<" << ent->d_name << ">>" << endl;
                                            if(strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0){
                                                continue;
                                            }
                                            //open dir, send text to worker
                                            char filepath[256] = "";
                                            strcat(filepath,dir);
                                            strcat(filepath,"/");
                                            strcat(filepath,ent->d_name);
                                            tempfile = fopen(filepath,"r");
                                            if(tempfile == NULL){
                                                perror("");
                                                continue;
                                            }
                                            send(writefd[i],filepath);
                                            while(fgets(buffer,MAXBUFF,tempfile) != NULL){
                                                //cout << "------Sending to worker: " << i << endl;
                                                send(writefd[i],buffer);
                                            }
                                        }
                                        closedir (direct);
                                    } 
                                    else{
                                        /* could not open directory */
                                        perror ("");
                                        return EXIT_FAILURE;
                                    }
                                }
                                free(dir);
                                dir = NULL;
                            }
                        }
                    }
                }
                //cout << "Command: " << cmd << endl;
                strcpy(copycmd,cmd);
                tok = strtok(cmd," \t\r\n");
                //send command to workers
                if(strcmp(tok,"SEARCH")==0){
                    //cout << "executing /search" << endl;
                    for(int i=0;i<numWorkers;i++){
                        send(writefd[i],copycmd);
                    }
                    double deadline=10;
                    // tok = strtok(NULL," \t\r\n");
                    // while(tok!=NULL){
                    //     if(strcmp(tok,"-d")==0){
                    //         tok = strtok(NULL," \t\r\n");
                    //         deadline = atoi(tok);
                    //         break;
                    //     }
                    //     tok = strtok(NULL," \t\r\n");
                    // }
                    int i=0,j=0,k=0;
                    char returnbuffer[MAXBUFF] = "";
                    char buffer[MAXBUFF] = "";
                    char copybuffer[MAXBUFF] = "";
                    int endtime = time(NULL) + deadline;
                    bool stopreading = false;
                    while(i<numWorkers){//round robin style read from workers
                        if(read(readfd[j],buffer,MAXBUFF) == MAXBUFF){
                            stopreading = false;
                            if(time(NULL) < endtime){
                                k++; 
                                while(1){
                                    strcpy(copybuffer,buffer);
                                    char *tok = strtok(copybuffer," \t");
                                    //cout << "READING: " << buffer << endl;
                                    if(strcmp(tok,"/start") == 0 && strcmp(buffer+strlen(buffer)-5," /end") == 0){
                                        strncpy(returnbuffer,buffer+7,strlen(buffer)-11);
                                        returnbuffer[strlen(buffer)-12] = '\0';
                                        if(strcmp(returnbuffer,"ENDOFSEARCH")==0){
                                            stopreading = true;
                                            break;
                                        }   
                                        else{
                                            //cout << "writing to pipe: " << returnbuffer << endl;
                                            if(write(wfd,returnbuffer,MAXBUFF)==-1)
                                                perror("write1");
                                        }    
                                    }
                                    while(read(readfd[j],buffer,MAXBUFF) != MAXBUFF);
                                }
                            }
                            else{
                                while(1){
                                    strcpy(copybuffer,buffer);
                                    char *tok = strtok(copybuffer," \t");
                                    //cout << "READING: " << buffer << endl;
                                    if(strcmp(tok,"/start") == 0 && strcmp(buffer+strlen(buffer)-5," /end") == 0){
                                        strncpy(returnbuffer,buffer+7,strlen(buffer)-11);
                                        returnbuffer[strlen(buffer)-12] = '\0';
                                        if(strcmp(returnbuffer,"ENDOFSEARCH")==0){
                                            stopreading = true;
                                            break;
                                        } 
                                    }
                                    while(read(readfd[j],buffer,MAXBUFF) != MAXBUFF);
                                }
                            }
                            i++;
                        }
                        j++;
                        if(j>=numWorkers){
                            j = j % numWorkers;
                        }
                    }
                    //cout << "Received answer from: " << k << " out of: " << numWorkers << " workers" << endl;
                    strcpy(returnbuffer,"ENDOFSEARCH");
                    //cout << "writing to pipe: " << returnbuffer << endl;
                    if(write(wfd,returnbuffer,MAXBUFF)==-1)
                        perror("write2");
                }
                else if(strcmp(tok,"/maxcount")==0){//returns "123 dir/filepath"
                    //cout << "executing /maxcount" << endl;
                    for(int i=0;i<numWorkers;i++){
                        send(writefd[i],copycmd);
                    }
                    tok = strtok(NULL," \t\r\n");
                    if(tok!=NULL){
                        int max=-1;
                        char maxfile[256] = "";
                        //cout << "keyword: " << tok << endl;
                        for(int i=0;i<numWorkers;i++){
                            received = receive(readfd[i]);
                            if(strcmp(received,"END")==0){
                                free(received);
                                continue;
                            }
                            tok = strtok(received," \n");
                            if(atoi(tok) > max){
                                max = atoi(tok);
                                tok = strtok(NULL," \n");
                                strcpy(maxfile,tok);
                            }
                            else if(atoi(tok) == max){
                                tok = strtok(NULL," \n");
                                if(strcmp(tok,maxfile) < 0){
                                    strcpy(maxfile,tok);
                                }
                            }
                            free(received);
                        }
                        if(maxfile!=NULL){
                            cout << "The file that contains keyword most times is: " << maxfile << endl;
                        }
                        else{
                            cout << "Keyword doesnt exist!!!" << endl;
                        }
                    }
                    else{
                        cout << "wrong arguments" << endl;
                    }
                }
                else if(strcmp(tok,"/mincount")==0){
                    //cout << "executing /mincount" << endl;
                    for(int i=0;i<numWorkers;i++){
                        send(writefd[i],copycmd);
                    }
                    if(tok!=NULL){
                        int min=1000000;
                        char minfile[256] = "";;
                        //cout << "keyword: " << tok << endl;
                        for(int i=0;i<numWorkers;i++){
                            received = receive(readfd[i]);
                            if(strcmp(received,"END")==0){
                                free(received);
                                continue;
                            }
                            tok = strtok(received," \n");
                            if(atoi(tok) < min){
                                min = atoi(tok);
                                tok = strtok(NULL," \n");
                                strcpy(minfile,tok);
                            }
                            else if(atoi(tok) == min){
                                tok = strtok(NULL," \n");
                                if(strcmp(tok,minfile) < 0){
                                    strcpy(minfile,tok);
                                }
                            }
                            free(received);
                        }
                        if(minfile!=NULL){
                            cout << "The file that contains keyword least times is: " << minfile << endl;
                        }
                        else{
                            cout << "Keyword doesnt exist!!!" << endl;
                        }
                    }
                    else{
                        cout << "wrong arguments" << endl;
                    }
                }
                else if(strcmp(tok,"/wc")==0){//receives numberof bytes,number of words,number of lines
                    //cout << "executing /wc" << endl;
                    int nochars=0,nowords=0,lines=0;
                    for(int i=0;i<numWorkers;i++){
                        send(writefd[i],copycmd);
                    }
                    for(int i=0;i<numWorkers;i++){
                        received = receive(readfd[i]);
                        tok = strtok(received," \n");
                        for(int j=0;j<3;j++){
                            if(j==0){
                                nochars += atoi(tok);
                            }                     
                            else if(j==1){
                                nowords += atoi(tok);
                            }
                            else if(j==2){
                                lines += atoi(tok);
                            }
                            tok = strtok(NULL," \n");
                        }
                        free(received);
                    }
                    cout << "Total number of bytes: " << nochars << endl;
                    cout << "Total number of words: " << nowords << endl;
                    cout << "Total number of lines: " << lines << endl;
                }
                else if(strcmp(tok,"/exit")==0){
                    int count=0;
                    for(int i=0;i<numWorkers;i++){
                        send(writefd[i],copycmd);
                        received = receive(readfd[i]);
                        count += atoi(received);
                        free(received);
                    }
                    //cout << "Exiting commands" << endl;
                    for(int i=0;i<numWorkers;i++){
                        close(readfd[i]);
                        close(writefd[i]);
                        sprintf(buf,"%d",i);
                        strcat(pipename,"tmp/pipeSRCW_");
                        strcat(pipename,buf);
                        //cout << "pipename = " << pipename << endl;
                        strcat(pipename2,"tmp/pipeSWCR_");
                        strcat(pipename2,buf);
                        //cout << "pipename2 = " << pipename2 << endl;
                        unlink(pipename);
                        unlink(pipename2);
                    }
                    memset(cmd,'\0',MAXBUFF);
                    free(workers);
                    fclose(file);
                    cout << "Total number of strings found in workers is: " << count << endl;
                    cout << "---Exiting PID = " << getpid() << endl;
                    return 0;
                }
                else{
                    cout << "Wrong command!!!" << endl;
                }
            }
            memset(cmd,'\0',MAXBUFF);
        }
    }
    cout << "---Exiting PID = " << getpid() << endl;
}