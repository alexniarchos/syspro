//intlist.h

class intlistnode{
    public:
        int noline;
        intlistnode *next;
        intlistnode(int);
};

class intlist{
    public:
        int no_nodes;
        intlistnode *head;
        int insert(int noline);
        int freelist();
        intlist();
};