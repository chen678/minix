#ifndef __PROTO_H
#define __PROTO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define HELLO_MESSAGE "Hello!\n"
#define WRITE_SIZE 10
#define BUFFER_MAX_SIZE 50

/** deque.c **/
typedef struct {
    int *arr;
    int head;
    int rear; 
    int size;
    int cnt;
} Deque;

Deque* initDeque();

bool insertFront(Deque* obj, int value);

bool insertLast(Deque* obj, int value);

bool deleteFront(Deque* obj);

bool deleteLast(Deque* obj);

int getFront(Deque* obj);

int getRear(Deque* obj);

bool isEmpty(Deque* obj);

bool isFull(Deque* obj);

void printDeque(Deque* obj);

void freeDeque(Deque* obj);

#endif /* __PROTO_H */
