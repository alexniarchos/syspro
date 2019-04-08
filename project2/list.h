#ifndef LIST_H
#define LIST_H

class listnode{
    public:
        int noline;
        int counter;
        listnode *next;
        listnode(int);
};

class list{
    public:
        int no_nodes;
        listnode *head;
        int insert(int noline);
        int freelist();
        list();
};

#endif