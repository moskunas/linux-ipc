/*
skipListK.h
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.20.2018

Kernel-space implementation of the userspace skipList.h program for skipListK.c (rest in peace me)
*/

#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <linux/kernel.h>

typedef struct mailbox
{

	struct mailbox *next ;
	unsigned char *message ;
	unsigned int messageLen ;

} mailbox ;

typedef struct slNode
{

	unsigned int slNodeID ; 
	unsigned int ownedBy ;
	int protect ;
	struct slNode **forwardList ; 
	struct mailbox *head , *tail ; 

} slNode ;


typedef struct skipList
{

	unsigned int level ;
	unsigned int maxLevel ;
	unsigned int promoteProb ;
	struct slNode *head ;

} skipList ;


asmlinkage long slmbx_init(unsigned int ptrs , unsigned int prob) ;


asmlinkage long slmbx_shutdown(void) ;


asmlinkage long slmbx_create(unsigned int id , int protected) ;


asmlinkage long slmbx_destroy(unsigned int id) ;


asmlinkage long slmbx_count(unsigned int id) ;


asmlinkage long slmbx_send(unsigned int id , const unsigned char *msg , unsigned int len) ;


asmlinkage long slmbx_recv(unsigned int id , unsigned char *msg , unsigned int len) ;


asmlinkage long slmbx_length(unsigned int id) ;


asmlinkage int slmbx_dump(void) ;


#endif
