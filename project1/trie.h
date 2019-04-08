#ifndef TRIE_H
#define TRIE_H
class postinglistnode{
    public:
        int id;
        int count;
        int dfcounter;
        postinglistnode* next;
        postinglistnode(int Id,int Count);
};

class trienode{
    public:
        char letter;
        trienode *child;
        trienode *sibling;
        postinglistnode *postlist;
        trienode();
};

void insertTrie(trienode *root,char *word,int id);

void freeTrie(trienode *cur);

void printTrie(trienode *root);//useful for debugging

void freePostlist(postinglistnode *ps);

postinglistnode* findPostingListof(trienode *root,char *word);

#endif