#ifndef TRIE_H
#define TRIE_H
#include "list.h"

class postinglistnode{
    public:
        char path[256];
        int fileid;
        list *mylist;//contains (noline,line,counter)
        postinglistnode* next;
        postinglistnode(char *Path,int Fileid);
};

class trienode{
    public:
        char letter;
        trienode *child;
        trienode *sibling;
        postinglistnode *postlist;
        trienode();
};

void insertTrie(trienode *root,char *word,char *path,int fileid,int noline);

void freeTrie(trienode *cur);

void printTrie(trienode *root);//useful for debugging

void freePostlist(postinglistnode *ps);

postinglistnode* findPostingListof(trienode *root,char *word);

#endif