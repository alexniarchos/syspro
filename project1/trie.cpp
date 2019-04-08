#include <iostream>
#include "trie.h"
#include <stdlib.h>
#include "string.h"

using namespace std;

postinglistnode::postinglistnode(int Id,int Count): id(Id),count(Count),next(NULL) {} 
trienode::trienode(): letter(-1),child(NULL),sibling(NULL),postlist(NULL) {}

void insertTrie(trienode *root,char *word,int id){// inserts a word into the trie saving information about document id into posting list
    trienode *temp = root,*father,*swap,*prev,*lastnode = NULL;
    int i=0;
    bool first = false, done = false;
    father = temp;
    temp = temp->child;
    if(temp == NULL){//insert first element in trie
        father->child = new trienode();
        father->child->letter = word[i];
        temp = father->child;
    }
    while(i<strlen(word)){
        first = false;
        done = false;
        if(temp!=NULL && temp->letter == word[i]){
            father = temp;
            temp = temp->child;
            i++;
            lastnode = father;
        }
        else if(temp == NULL){
            father->child = new trienode();
            father->child->letter = word[i++];
            father = father->child;
            temp = father->child;
            lastnode = father;
        }
        else{
            while(word[i] > temp->letter){
                first = true;
                if(temp->sibling!=NULL){//search the horizontal list
                    prev = temp;
                    temp = temp->sibling;
                }
                else{//end of list insert letter here
                    temp->sibling = new trienode();
                    temp->sibling->letter = word[i++];
                    father = temp->sibling;
                    lastnode = father;
                    temp = temp->sibling->child;
                    done = true;
                    break;
                }
            }
            if(temp!=NULL && temp->letter == word[i]){
                father = temp;
                temp = temp->child;
                i++;
                lastnode = father;
            }
            else if(!first){//swapping current letter with first
                swap = father->child;
                father->child = new trienode();
                father->child->letter = word[i++];
                father->child->sibling = swap;
                temp = father->child->child;
                father = father->child;
                lastnode = father;
            }
            else if(!done){//didnt insert letter previously
                prev->sibling = new trienode();
                prev->sibling->letter = word[i++];
                prev->sibling->sibling = temp;
                temp = prev->sibling->child;
                father = prev->sibling;
                lastnode = prev->sibling;
            }
        }
    }
    //posting list update
    if(lastnode->postlist == NULL){//first time
        lastnode->postlist = new postinglistnode(id,1);
        lastnode->postlist->dfcounter = 1;
        return;
    }
    postinglistnode *temp2=lastnode->postlist;
    postinglistnode *prev2;
    while(temp2!=NULL){
        if(temp2->id == id){
            temp2->count++;
            return;
        }
        prev2 = temp2;
        temp2 = temp2->next;
    }
    prev2->next = new postinglistnode(id,1);
    lastnode->postlist->dfcounter++;
}

void freeTrie(trienode *cur){
    if(cur == NULL)
        return;
    trienode *child,*sibling;
    child = cur->child;
    sibling = cur->sibling;
    if(cur->postlist!=NULL)
        freePostlist(cur->postlist);
    delete cur;
    freeTrie(child);
    freeTrie(sibling);
}

void printTrie(trienode *root){
    if(root == NULL){
        cout << endl;
        return;
    }
    cout << root->letter;
    printTrie(root->child);
    if(root->sibling!=NULL){
        printTrie(root->sibling);
    }
}

void freePostlist(postinglistnode *ps){
    if(ps == NULL)
        return;
    postinglistnode *next;
    next = ps->next;
    delete ps;
    freePostlist(next);
}

postinglistnode* findPostingListof(trienode *root,char *word){//return posting list pointer of a given word
    trienode *temp = root;
    if(root == NULL){
        cout << "Trie is empty!!!" << endl;
        return NULL;
    }
    temp = temp->child;
    int i=0,wordlen;
    wordlen = strlen(word);
    while(temp != NULL && i<wordlen){
        if(temp->letter == word[i]){
            if(temp->child != NULL && i+1<wordlen){
                temp = temp->child;
                i++;
            }
            else{
                break;
            }
        }
        else{
            temp = temp->sibling;
            while(temp!=NULL){
                if(temp->letter == word[i]){
                    if(temp->child != NULL && i+1<wordlen){
                        temp = temp->child;
                        i++;
                    }
                    break;
                }
                else{
                    temp = temp->sibling;
                }
            }
            if(temp == NULL){
                //cout << "Word: " << word << " doesnt exist" << endl;
            }
        }
    }
    if(temp!=NULL && temp->postlist!=NULL && i+1==wordlen){
        return temp->postlist;
    }
    else{
        cout << "posting list was not found!" << endl;
        return NULL;
    }
}