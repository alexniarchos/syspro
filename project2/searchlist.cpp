//searchlist.cpp
#include "searchlist.h"
#include <stdlib.h>

searchlistnode::searchlistnode(char *Filepath,int Fileid): filepath(Filepath),fileid(Fileid),next(NULL){
    nolinelist = new intlist();
}
searchlist::searchlist(): no_nodes(0),head(NULL){}

int searchlist::insert(char *filepath,int fileid,int noline){
    searchlistnode *temp = this->head;
    if(temp == NULL){
        this->head = new searchlistnode(filepath,fileid);
        this->head->nolinelist->insert(noline);
        this->no_nodes++;
        return 1;
    }
    else{
        while(temp!=NULL){
            if(temp->fileid == fileid){//doenst need to insert the same noline again
                temp->nolinelist->insert(noline);
                return 0;
            }
            else if(temp->next == NULL){
                temp->next = new searchlistnode(filepath,fileid);
                temp->next->nolinelist->insert(noline);
                this->no_nodes++;
                return 1;
            }
            temp = temp->next;
        }
    }
}

int searchlist::freelist(){
    searchlistnode *head = this->head;
    searchlistnode *temp = head;
    while (temp != NULL) {
        head = head->next;
        temp->nolinelist->freelist();
        delete(temp->nolinelist);
        delete temp;
        temp = head;
    }
}