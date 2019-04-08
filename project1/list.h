class listnode{
    public:
        int doc_id;
        listnode *next;
        listnode(int id);
};

class list{
    public:
        int no_nodes;
        listnode *head;
        list();
        int insert(int id);
        int freelist();
};