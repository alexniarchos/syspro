#include "trie.h"

int search(char **files[],trienode *root,int writefd,char **queries);

void maxcount(trienode *root,int writefd,char *keyword);

void mincount(trienode *root,int writefd,char *keyword);

void wc(int writefd,char ***files,int nofiles,int *nolines);

char* getformattime();

int send(int writefd,char *buffer);

char *receive(int readfd);
