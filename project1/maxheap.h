typedef struct heap_entry{
    float score;
    int doc_id;
}heap_entry;

class maxheap
{
    heap_entry **heap; // pointer to array of elements in heap
    int capacity; // maximum possible size of min heap
    int heap_size; // Current number of elements in min heap
    public:
        // Constructor
        maxheap(int capacity);
    
        int parent(int i);
    
        // to get index of left child of node at index i
        int left(int i);
    
        // to get index of right child of node at index i
        int right(int i);
    
        // to extract the root which is the minimum element
        heap_entry* extract();
    
        // Inserts a new key 'k'
        void insert(heap_entry *);

        //return position of biggest child in heap array
        int biggestChild(int ,int );

        void swap(int,int);

        void destroy();
};