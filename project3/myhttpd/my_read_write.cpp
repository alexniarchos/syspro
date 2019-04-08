#include <unistd.h>
#include <iostream>
#include <string.h>
#include <stdio.h>

using namespace std;

int mywrite(int fd,char* buffer,int size){
    int count=0;
    while(count < size){
        count += write(fd,buffer+count,size-count);
    }
    return 1;
}

int myread(int fd,char* buffer,int size,const char *endstring){
    int count=0,readbytes=0;
    while(count < size){
        readbytes = read(fd,buffer+count,size-count);
        if(readbytes == 0){
            perror("read connection end");
            return 0;
        }
        count += readbytes;
        if(endstring == NULL){
            continue;
        }
        else if(strstr(buffer,endstring)!=NULL){
            break;
        }
    }
    return count;
}