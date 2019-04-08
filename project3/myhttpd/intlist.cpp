//intlist.cpp
#include "intlist.h"
#include <stdlib.h>

intlistnode::intlistnode(int FD): fd(FD),next(NULL){}
intlist::intlist(): totalitems(0),head(NULL){}

void intlist::insert(int fd){
    intlistnode *temp = this->head;
    if(temp == NULL){
        this->head = new intlistnode(fd);
    }
    else{
        while(temp->next!=NULL){
            temp = temp->next;
        }
        temp->next = new intlistnode(fd);
    }
    totalitems++;
}

int intlist::remove(){//remove first element in list
    int retval = this->head->fd;
    intlistnode *temp = this->head;
    this->head = this->head->next;
    delete temp;
    totalitems--;
    return retval;
}

int intlist::freelist(){
    intlistnode *head = this->head;
    intlistnode *temp = head;
    while (temp != NULL) {
        head = head->next;
        delete temp;
        temp = head;
    }
    return 0;
}