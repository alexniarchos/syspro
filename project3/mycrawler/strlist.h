//strlist.h

class strlistnode{
    public:
        char* url;
        strlistnode *next;
        strlistnode(char*);
};

class strlist{
    public:
        strlistnode *head;
        int totalitems;
        void insert(char*);
        char* remove();
        int search(char*);
        int freelist();
        strlist();
};