#include "list.h"
#include <stdlib.h>

listnode::listnode(int Noline): noline(Noline),next(NULL){}
list::list(): no_nodes(0),head(NULL){}

int list::insert(int noline){
    listnode *temp = this->head;
    if(temp == NULL){
        this->head = new listnode(noline);
        this->head->counter++;
        this->no_nodes++;
        return 1;
    }
    else{
        while(temp!=NULL){
            if(temp->noline == noline){//doenst need to insert the same noline again
                temp->counter++;
                return 0;
            }
            else if(temp->next == NULL){
                temp->next = new listnode(noline);
                temp->next->counter++;
                this->no_nodes++;
                return 1;
            }
            temp = temp->next;
        }
    }
}

int list::freelist(){
    listnode *head = this->head;
    listnode *temp = head;
    while (temp != NULL) {
        head = head->next;
        delete temp;
        temp = head;
    }
}