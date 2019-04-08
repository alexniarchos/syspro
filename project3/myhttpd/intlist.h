//intlist.h

class intlistnode{
    public:
        int fd;
        intlistnode *next;
        intlistnode(int);
};

class intlist{
    public:
        intlistnode *head;
        int totalitems;
        void insert(int);
        int remove();
        int freelist();
        intlist();
};