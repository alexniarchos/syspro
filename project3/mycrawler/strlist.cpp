//strlist.cpp
#include "strlist.h"
#include <stdlib.h>
#include <string.h>

strlistnode::strlistnode(char *URL): next(NULL){
    this->url = (char*)malloc(strlen(URL)+1);
    strcpy(this->url,URL);
}
strlist::strlist(): totalitems(0),head(NULL){}

void strlist::insert(char *url){
    strlistnode *temp = this->head;
    if(temp == NULL){
        this->head = new strlistnode(url);
    }
    else{
        while(temp->next!=NULL){
            temp = temp->next;
        }
        temp->next = new strlistnode(url);
    }
    totalitems++;
}

char* strlist::remove(){//remove first element in list
    char *retval = this->head->url;
    strlistnode *temp = this->head;
    this->head = this->head->next;
    delete temp;
    totalitems--;
    return retval;
}

int strlist::search(char* str){//searches string in list, returns 1 if str found
    strlistnode *temp = this->head;
    while (temp != NULL) {
        if(strcmp(temp->url,str)==0)
            return 1;
        temp = temp->next;
    }
    return 0;
}

int strlist::freelist(){
    strlistnode *head = this->head;
    strlistnode *temp = head;
    while (temp != NULL) {
        head = head->next;
        free(temp->url);
        delete temp;
        temp = head;
    }
    return 0;
}