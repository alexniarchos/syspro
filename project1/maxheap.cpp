#include "maxheap.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

// index of parent of node at index i
int maxheap::parent(int i) { return (i-1)/2; }

// index of left child of node at index i
int maxheap::left(int i) { return (2*i + 1); }

// index of right child of node at index i
int maxheap::right(int i) { return (2*i + 2); }

maxheap::maxheap(int cap)
{
    heap_size = 0;
    capacity = cap;
    heap = new heap_entry*[cap];
    for(int i=0;i<cap;i++){
        heap[i] = NULL;
    }
}

void maxheap::swap(int i1,int i2){
    heap_entry *temp = heap[i1];
    heap[i1] = heap[i2];
    heap[i2] = temp;
}

void maxheap::insert(heap_entry *k)
{
    if (heap_size == capacity)
    {
        cout << "Heap is FULL, cant insertKey!!!\n";
        delete k;
        return;
    }
 
    // First insert the new key at the end
    heap_size++;
    int i = heap_size - 1;
    heap[i] = k;
 
    // Fix the min heap property if it is violated
    while (i != 0 && heap[parent(i)]->score < heap[i]->score)
    {
       swap(i,parent(i));
       i = parent(i);
    }
}

int maxheap::biggestChild(int i1,int i2){//return the process with the smallest priority
    if(i1 < heap_size && i2 < heap_size){//if inside of array bounds
        if(heap[i1]->score > heap[i2]->score){
            return i1;
        }
        else{
            return i2;
        }
    }
    else if(i1 < heap_size){//if there isn't a right child return the first one
        return i1;
    }
    return -1; //no childs
}

heap_entry* maxheap::extract()
{
    int i = 0,maxChild;
    if (heap_size <= 0)
        return NULL;
    if (heap_size == 1)
    {
        heap_size--;
        return heap[0];
    }
 
    // Store the minimum value, and remove it from heap
    heap_entry *root = heap[0];
    heap[0] = heap[heap_size-1];
    heap_size--;
    while(1){
        maxChild = biggestChild(2*i+1,2*i+2);
        if(maxChild != -1 && heap[maxChild]->score > heap[i]->score){//swap smallest child if it has smaller priority than the parent
            swap(maxChild,i);
            i = maxChild;
        }
        else{
            break;
        }
    }
    return root;
}

void maxheap::destroy(){
    for(int i=0; i<heap_size; i++){
        if(heap[i]!=NULL){
            delete heap[i];
        }
    }
    if(heap!=NULL)
        delete[] heap;
}