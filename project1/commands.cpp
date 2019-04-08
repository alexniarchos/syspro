#include <iostream>
#include "commands.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include "string.h"
#include <stdio.h>
#include <math.h>

#include "trie.h"
#include "list.h"
#include "maxheap.h"
#include "globals.h"

using namespace std;

void printdf(char *word,int i,trienode *temp){//called by df() to print t
    word[i] = temp->letter;
    cout << word[i];
    if(temp->postlist != NULL){
        cout << " " << temp->postlist->dfcounter << endl;
        if(temp->child!=NULL){
            for(int j=0;j<=i;j++){
                cout << word[j];
            }
            printdf(word,i+1,temp->child);
        }
    }
    else if(temp->child!=NULL){
        printdf(word,i+1,temp->child);
    }
    if(temp->sibling!=NULL){
        for(int j=0;j<i;j++){
            cout << word[j];
        }
        printdf(word,i,temp->sibling);
    }
}

void df(trienode* root,char* word){
    postinglistnode *pl;
    if(word==NULL){//search all words
        char str[512];
        printdf(str,0,root->child);
    }
    else{//search only word
        pl = findPostingListof(root,word);
        if(pl != NULL)
            cout << word << " " << pl->dfcounter << endl;
    }
}

int tf(int id,char *word, trienode *root){
    //cout << "searching for: " << word << endl;
    postinglistnode *postlist = findPostingListof(root,word);
    if(postlist != NULL){
        postinglistnode *temp=postlist;
        while(temp!=NULL){
            if(temp->id == id){
                //cout << word << " " << temp->count << endl;
                return temp->count;
            }
            temp = temp->next;
        }
        //cout << "'" << word << "'" << " doesnt exist in id= " << id << endl; 
    }
    return 0;
}

int countwords(char* doc){//count words of a given document id
    int count = 0;
    char temp[strlen(doc)];
    strcpy(temp,doc);
    char *tok = strtok(temp," \t\r\n");
    while(tok != NULL){
        count++;
        tok = strtok(NULL," \t\r\n");
    }
    return count;
}

float IDF(trienode* root,char* word){//calulate IDF
    postinglistnode *postlist = findPostingListof(root,word);
    if(postlist != NULL){
        //cout << "postlist->dfcounter = " << postlist->dfcounter << endl;
        return log10((float)(linecounter - postlist->dfcounter + 0.5)/(postlist->dfcounter + 0.5));
    }
    else{
        return log10((float)(linecounter + 0.5)/(0.5));
    }
}

float score(char** docs,int id,trienode *trieroot,char **words,int size){//calculate score
    float k1 = 1.2, b = 0.75,score=0;
    int D = countwords(docs[id]);
    int totalD = 0;
    for(int i=0;i<linecounter;i++){
        totalD += countwords(docs[i]);
    }
    float avgdl = (float)totalD/linecounter;
    for(int i=0;i<size;i++){
        int termfreq = tf(id,words[i],trieroot);
        //cout << "tf = " << termfreq << endl << "D = " << D << endl << "linecounter = " << linecounter << endl << "avgdl = " << avgdl << endl << "IDF = " << (float)IDF(trieroot,words[i]) << endl;
        score += (float)IDF(trieroot,words[i])*(termfreq*(k1+1)/(float)(termfreq+k1*(1-b+b*((float)D/avgdl))));
    }
    return score;
}

int isdelimiter(char c){
    if(c == ' ' || c == '\t' || c == '\n' || c == '\0'){
        return 1;
    }
    return 0;
}

int getDigits(int num)//returns number of digits
{
    if(num == 0){
        return 1;
    }
    return (int) log10(num) + 1;
}

void search(trienode *trieroot,char **docs,char **words,int size){
    list *root = new list();
    for(int i=0;i<size;i++){
        postinglistnode *postlist = findPostingListof(trieroot,words[i]);
        if(postlist != NULL){//read posting list
            postinglistnode *temp=postlist;
            while(temp!=NULL){
                root->insert(temp->id);//insert doc ids that need to calculate their scores
                temp = temp->next;
            }
        }
        else{
            //cout << "word doesnt have a posting list" << endl;
        }
    }
    
    //for each document calculate score and insert score-doc_id into maxheap
    listnode *temp = root->head;
    maxheap heap = maxheap(root->no_nodes);
    while(temp!=NULL){
        heap_entry *he = new heap_entry;
        he->doc_id = temp->doc_id;
        he->score = score(docs,temp->doc_id,trieroot,words,size);
        heap.insert(he);
        temp = temp->next;
    }

    int i=0,counter=0;
    heap_entry *extracted = heap.extract();
    while(extracted != NULL && i<k){
        //cout << "Extracted id = " << extracted->doc_id << " with score = " << extracted->score << endl;
        char *doc = docs[extracted->doc_id];
        //print the highlight symbols
        char symbols[strlen(doc)];
        strcpy(symbols,doc);
        for(int k=0;k<size;k++){
            char *word = words[k];
            char *start = symbols;
            char *end;
            int length = strlen(word);
            int wordi=0,doci=0,counter=0;
            while((start = strstr(start,word))!=NULL){//for each extracted word locate it into the document and highlight by replacing letter with ^ symbol
                end = start + length;
                if(start == symbols || isdelimiter(*(start-1))){
                    if(isdelimiter(*end)){
                        while(start<end){
                            *start = '^';
                            start++;
                        }
                    }
                }
                start = end;
            }
        }

        for(int k=0;k<strlen(doc);k++){//clear the array and leave only the highlight symbols
            if(symbols[k]!='^')
                symbols[k] = ' ';
        }

        int offset = getDigits(i+1) + 1 + 2 + getDigits(linecounter) + 2+8+1;//calculate printing offset
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);//get terminal width
        w.ws_col -= offset;
        int lines = strlen(doc)/w.ws_col;
        if(lines == 0 && strlen(doc)>0){//if documents fits into terminal width
            cout << i+1 << "." << "(";
            for(int k=0;k<getDigits(linecounter)-getDigits(extracted->doc_id);k++){
                cout << " ";
            }
            cout << extracted->doc_id << ")";
            printf("[%8.4f] ",extracted->score);
            cout << docs[extracted->doc_id] << endl;
            for(int k=0;k<offset;k++){
                cout << " ";
            }
            for(int k=0;k<strlen(doc);k++){
                cout << symbols[k] ;
            }
            cout << endl;
        }
        else{//printing takes more than 1 line
            for(int k=0;k<lines;k++){
                if(k==0){
                    cout << i+1 << "." << "(";
                    for(int j=0;j<getDigits(linecounter)-getDigits(extracted->doc_id);j++){
                        cout << " ";
                    }
                    cout << extracted->doc_id << ")";
                    printf("[%8.4f] ",extracted->score);
                }
                else{
                    for(int j=0;j<offset;j++){
                        cout << " ";
                    }
                }
                for(int l=k*w.ws_col;l<(k+1)*w.ws_col;l++){
                    cout << docs[extracted->doc_id][l];
                }
                cout << endl;
                for(int j=0;j<offset;j++){
                    cout << " ";
                }
                for(int l=k*w.ws_col;l<(k+1)*w.ws_col;l++){
                    cout << symbols[l];
                }
                cout << endl;
            }
            if(strlen(doc) % w.ws_col > 0){
                for(int j=0;j<offset;j++){
                    cout << " ";
                }
                for(int k=lines*w.ws_col;k<lines*w.ws_col+strlen(doc)%w.ws_col;k++){
                    cout << docs[extracted->doc_id][k];
                }
                cout << endl;
                for(int j=0;j<offset;j++){
                    cout << " ";
                }
                for(int k=lines*w.ws_col;k<lines*w.ws_col+strlen(doc)%w.ws_col;k++){
                    cout << symbols[k];
                }
                cout << endl;
            }
        }
        i++;
        delete extracted;
        extracted = heap.extract();
    }
    if(extracted!=NULL)
        delete extracted;
    heap.destroy();
    root->freelist();
    delete root;
}