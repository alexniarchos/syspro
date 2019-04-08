//intlist.cpp
#include "intlist.h"
#include <stdlib.h>

intlistnode::intlistnode(int Noline): noline(Noline),next(NULL){}
intlist::intlist(): no_nodes(0),head(NULL){}

int intlist::insert(int noline){
    intlistnode *temp = this->head;
    if(temp == NULL){
        this->head = new intlistnode(noline);
        this->no_nodes++;
        return 1;
    }
    else{
        while(temp!=NULL){
            if(temp->noline == noline){//doenst need to insert the same noline again
                return 0;
            }
            else if(temp->next == NULL){
                temp->next = new intlistnode(noline);
                this->no_nodes++;
                return 1;
            }
            temp = temp->next;
        }
    }
}

int intlist::freelist(){
    intlistnode *head = this->head;
    intlistnode *temp = head;
    while (temp != NULL) {
        head = head->next;
        delete temp;
        temp = head;
    }
}