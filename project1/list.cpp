#include "list.h"
#include <stdlib.h>

listnode::listnode(int id): doc_id(id),next(NULL){}
list::list(): no_nodes(0),head(NULL){}

int list::insert(int id){
    listnode *temp = this->head;
    if(temp == NULL){
        this->head = new listnode(id);
        this->no_nodes++;
        return 1;
    }
    else{
        while(temp!=NULL){
            if(temp->doc_id == id){//doenst need to insert the same id again
                return 0;
            }
            else if(temp->next == NULL){
                temp->next = new listnode(id);
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