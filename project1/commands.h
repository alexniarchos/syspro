#include "trie.h"

void printdf(char *word,int i,trienode *temp);

int tf(int id,char *word, trienode *root);

void df(trienode* root,char* word);

int countwords(char* doc);

float IDF(trienode* root,char* word);

float score(char** docs,int id,trienode *trieroot,char **words,int size);

int isdelimiter(char c);

int getDigits(int num);

void search(trienode *trieroot,char **docs,char **words,int size);