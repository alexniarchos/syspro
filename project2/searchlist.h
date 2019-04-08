//searchlist.h
#include "intlist.h"

class searchlistnode{
    public:
        char *filepath;
        int fileid;
        intlist *nolinelist;
        searchlistnode *next;
        searchlistnode(char *,int);
};

class searchlist{
    public:
        int no_nodes;
        searchlistnode *head;
        int insert(char *,int ,int );
        int freelist();
        searchlist();
};