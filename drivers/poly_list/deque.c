#include "proto.h"

/* State management variables. */
bool dequeIsEmpty;

Deque* initDeque() {
    Deque* obj=malloc(sizeof(Deque));
    obj->arr=malloc(sizeof(int)*BUFFER_MAX_SIZE);
    obj->size=BUFFER_MAX_SIZE;
    obj->head=BUFFER_MAX_SIZE-1;
    obj->rear=0;
    obj->cnt=0;
    dequeIsEmpty = true;
    return obj;
}
bool isFull(Deque* obj) {
	return obj->cnt==obj->size;
}

bool isEmpty(Deque* obj) {
	dequeIsEmpty = (obj->cnt==0);
	return obj->cnt==0;
}

bool insertFront(Deque* obj, int value) {
	if(!isFull(obj)){  
		obj->arr[obj->head]=value;
		obj->head=(obj->head-1+obj->size)%obj->size;
		obj->cnt++;
		return true;
	}
	return false;
}

bool insertLast(Deque* obj, int value) {
	if(!isFull(obj)){  
		obj->arr[obj->rear]=value;
		obj->rear=(obj->rear+1)%obj->size;
		obj->cnt++;
		return true;
	}
	return false;
}

bool deleteFront(Deque* obj) {
	if(!isEmpty(obj)){
		obj->head=(obj->head+1)%obj->size;
		obj->cnt--;
		return true;
	}
	return false;
}

bool deleteLast(Deque* obj) {
	if(!isEmpty(obj)) {
		obj->rear=(obj->rear-1+obj->size)%obj->size;
		obj->cnt--;
		return true;
	}
	return false;
}

int getFront(Deque* obj) {
	if(!isEmpty(obj)){
		return obj->arr[(obj->head+1)%obj->size];
	}
	return -1;
}

int getRear(Deque* obj) {
	if(!isEmpty(obj)){
		return obj->arr[(obj->rear-1+obj->size)%obj->size];
	}
	return -1;
}

void freeDeque(Deque* obj) {
    free(obj->arr);
    free(obj);
}

void printDeque(Deque* obj){
	if(!isEmpty(obj)){
		int i = 0, walker = obj->head;
		for(;i < (obj->cnt); i++){
			walker++;
			printf("%d ", obj->arr[walker % (obj->size)]);
		}
		
	}
	printf("Done.\n");
}

