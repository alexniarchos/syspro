#include <iostream>
#include "commands.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include "string.h"
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>

#include "trie.h"
#include "list.h"
#include "searchlist.h"
#include "globals.h"

using namespace std;

char* getformattime(){
    char* buff = (char*)malloc(100); // space enough for DD/MM/YYYY HH:MM:SS and terminator
    struct tm *sTm;
    time_t now;

    now = time(NULL);
    sTm = gmtime (&now);

    strftime (buff, 100, "%Y-%m-%d %H:%M:%S", sTm);
    return buff;
}

int send(int writefd,char *buffer){
    char sendstring[strlen(buffer)+20] = "";
    strcat(sendstring,"/start ");
    strcat(sendstring,buffer);
    strcat(sendstring," /end");
    //cout << "SENDING: " << sendstring << endl;
    while(write(writefd,sendstring,MAXBUFF) != MAXBUFF);
    return 0;
}

char *receive(int readfd){
    char buffer[MAXBUFF] = "";
    char *returnbuffer = (char*)malloc(MAXBUFF);
    char copybuffer[MAXBUFF] = "";
    while(1){
        if(read(readfd,buffer,MAXBUFF) == MAXBUFF){
            strcpy(copybuffer,buffer);
            char *tok = strtok(copybuffer," \t");
            //cout << "READING: " << buffer << endl;
            if(strcmp(tok,"/start") == 0 && strcmp(buffer+strlen(buffer)-5," /end") == 0){
                strncpy(returnbuffer,buffer+7,strlen(buffer)-11);
                returnbuffer[strlen(buffer)-12] = '\0';
                //cout << "returnbuffer = " << returnbuffer << endl;
                return returnbuffer;
            }
            cout << "receive returning NULL" << endl;
            return NULL;
        }
        
    }
}

int search(char **files[],trienode *root,int writefd,char **queries){
    // cout << "Queries are: " << endl;
    // for(int i=0;i<QUERIES;i++){
    //     if(queries[i] == NULL){
    //         break;
    //     }
    //     else{
    //         cout << queries[i] << endl;
    //     }
    // }
    int count=0;
    postinglistnode *ps;
    listnode *plist;
    searchlist *sl = new searchlist();
    char *ftime = NULL;
    char logbuff[MAXBUFF] = "";
    for(int i=0;i<QUERIES;i++){
        if(queries[i] == NULL)
            break;
        else{
            ps = findPostingListof(root,queries[i]);
            if(ps!=NULL){
                count++;//count queries that matched
                ftime = getformattime();
                strcat(logbuff,ftime);
                free(ftime);
                strcat(logbuff," : search : ");
                strcat(logbuff,queries[i]);
                strcat(logbuff," : ");
                while(ps!=NULL){
                    plist = ps->mylist->head;
                    while(plist!=NULL){
                        sl->insert(ps->path,ps->fileid,plist->noline);
                        //cout << "inserting " << ps->path << " " << plist->noline << endl;
                        plist = plist->next;
                    }
                    strcat(logbuff,ps->path);
                    strcat(logbuff," ");
                    ps = ps->next;
                }
                strcat(logbuff,"\n");
            }
        }
    }
    searchlistnode *temp = sl->head;
    intlistnode *temp2;
    char returnbuffer[MAXBUFF] = "";
    char numbuff[256] = "";
    while(temp!=NULL){
        //cout << "Filepath: " << temp->filepath << endl;
        strcat(returnbuffer,"Filepath: ");
        strcat(returnbuffer,temp->filepath);
        strcat(returnbuffer,"\n");
        temp2 = temp->nolinelist->head;
        while(temp2!=NULL){
            //cout << "Line id: " << temp2->noline << endl;
            sprintf(numbuff,"%d",temp2->noline);
            strcat(returnbuffer,"Line id: ");
            strcat(returnbuffer,numbuff);
            strcat(returnbuffer,"\n");
            //cout << "Line: " << files[temp->fileid][temp2->noline] << endl;
            strcat(returnbuffer,"Line: ");
            strcat(returnbuffer,files[temp->fileid][temp2->noline]);
            strcat(returnbuffer,"\n");
            send(writefd,returnbuffer);
            returnbuffer[0] = '\0';
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
    //open logfile and write
    char logname[128] = "log/worker_";
    sprintf(numbuff,"%d",getpid());
    strcat(logname,numbuff);
    //cout << "--------creating " << logname << endl;
    mkdir("log",0777);
    FILE *pfile = fopen(logname,"a");
    if(pfile!=NULL){
        fwrite (logbuff , sizeof(char), strlen(logbuff), pfile);
        fclose(pfile);
    }
    strcpy(returnbuffer,"ENDOFSEARCH");
    send(writefd,returnbuffer);
    //free
    sl->freelist();
    delete(sl);
    //cout << "Exiting search" << endl;
    return count;
}

void maxcount(trienode *root,int writefd,char *keyword){
    int max = 0,sum = 0;
    char *filepath = NULL;
    postinglistnode *ps = findPostingListof(root,keyword);
    if(ps==NULL){//keyword doesnt exist in trie
        char endbuf[] = "END";
        send(writefd,endbuf);
        return;
    }
    while(ps!=NULL){
        listnode *plist = ps->mylist->head;
        sum = 0;
        while(plist!=NULL){
            sum += plist->counter;
            //cout << "sum in file is: " << sum << " " << plist->noline << " " << ps->path << endl;
            plist = plist->next;
        }
        if(sum > max){
            max = sum;
            filepath = ps->path;
        }
        ps = ps->next;
    }
    char returnbuffer[MAXBUFF] = "";
    char numbuff[256] = "";
    sprintf(numbuff,"%d",max);
    strcat(returnbuffer,numbuff);
    strcat(returnbuffer," ");
    strcat(returnbuffer,filepath);
    send(writefd,returnbuffer);
    char logbuff[MAXBUFF] = "";
    //open logfile and write
    char logname[128] = "log/worker_";
    sprintf(numbuff,"%d",getpid());
    strcat(logname,numbuff);
    //cout << "--------creating " << logname << endl;
    mkdir("log",0777);
    FILE *pfile = fopen(logname,"a");
    if(pfile!=NULL){
        char *ftime = getformattime();
        strcat(logbuff,ftime);
        free(ftime);
        strcat(logbuff," : maxcount : ");
        strcat(logbuff,keyword);
        strcat(logbuff," : ");
        strcat(logbuff,filepath);
        strcat(logbuff,"\n");
        fwrite (logbuff , sizeof(char), strlen(logbuff), pfile);
        fclose(pfile);
    }
}

void mincount(trienode *root,int writefd,char *keyword){
    int min = 1000000,sum = 0;
    char *filepath;
    postinglistnode *ps = findPostingListof(root,keyword);
    if(ps==NULL){//keyword doesnt exist in trie
        char endbuf[] = "END";
        send(writefd,endbuf);
        return;
    }
    while(ps!=NULL){
        listnode *plist = ps->mylist->head;
        sum = 0;
        while(plist!=NULL){
            sum += plist->counter;
            //cout << "sum in file is: " << sum << " " << plist->noline << " " << ps->path << endl;
            plist = plist->next;
        }
        if(sum < min){
            min = sum;
            filepath = ps->path;
        }
        ps = ps->next;
    }
    char returnbuffer[MAXBUFF] = "";
    char numbuff[256] = "";
    sprintf(numbuff,"%d",min);
    strcat(returnbuffer,numbuff);
    strcat(returnbuffer," ");
    strcat(returnbuffer,filepath);
    send(writefd,returnbuffer);

    char logbuff[MAXBUFF] = "";
    //open logfile and write
    char logname[128] = "log/worker_";
    sprintf(numbuff,"%d",getpid());
    strcat(logname,numbuff);
    //cout << "--------creating " << logname << endl;
    mkdir("log",0777);
    FILE *pfile = fopen(logname,"a");
    if(pfile!=NULL){
        char *ftime = getformattime();
        strcat(logbuff,ftime);
        free(ftime);
        strcat(logbuff," : mincount : ");
        strcat(logbuff,keyword);
        strcat(logbuff," : ");
        strcat(logbuff,filepath);
        strcat(logbuff,"\n");
        fwrite (logbuff , sizeof(char), strlen(logbuff), pfile);
        fclose(pfile);
    }
}

void wc(int writefd,char ***files,int nofiles,int *nolines){
    int nochars=0,nowords=0,lines=0;
    char *tok;
    for(int i=0;i<nofiles;i++){
        for(int j=0;j<nolines[i];j++){
            nochars+=strlen(files[i][j]);
            char buffer[MAXBUFF] = "";
            strcpy(buffer,files[i][j]);
            tok = strtok(buffer," \t\n");
            while(tok!=NULL){
                nowords++;
                tok = strtok(NULL," \t\n");
            }
        }
        lines += nolines[i];
    }
    char returnbuffer[MAXBUFF] = "";
    char numbuff[256] = "";
    sprintf(numbuff,"%d",nochars);
    strcat(returnbuffer,numbuff);
    strcat(returnbuffer," ");
    sprintf(numbuff,"%d",nowords);
    strcat(returnbuffer,numbuff);
    strcat(returnbuffer," ");
    sprintf(numbuff,"%d",lines);
    strcat(returnbuffer,numbuff);
    send(writefd,returnbuffer);

    char logbuff[MAXBUFF] = "";
    //open logfile and write
    char logname[128] = "log/worker_";
    sprintf(numbuff,"%d",getpid());
    strcat(logname,numbuff);
    //cout << "--------creating " << logname << endl;
    mkdir("log",0777);
    FILE *pfile = fopen(logname,"a");
    if(pfile!=NULL){
        char *ftime = getformattime();
        strcat(logbuff,ftime);
        free(ftime);
        strcat(logbuff," : wc : ");
        sprintf(numbuff,"%d",nochars);
        strcat(logbuff,numbuff);
        strcat(logbuff," : ");
        sprintf(numbuff,"%d",nowords);
        strcat(logbuff,numbuff);
        strcat(logbuff," : ");
        sprintf(numbuff,"%d",lines);
        strcat(logbuff,numbuff);
        strcat(logbuff,"\n");
        fwrite (logbuff , sizeof(char), strlen(logbuff), pfile);
        fclose(pfile);
    }
}