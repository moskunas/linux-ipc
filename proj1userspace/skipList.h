/*
skipList.h
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.10.2018

Implementation for user-space skip list test implementation before porting into kernel space.
*/

#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdlib.h>
#include <stdio.h>

typedef struct mailbox
{

	struct mailbox *next ;
	const unsigned char *message ;

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


long slmbx_init(unsigned int ptrs , unsigned int prob) ;


long slmbx_shutdown(void) ;


long slmbx_create(unsigned int id , int protected) ;


long slmbx_destroy(unsigned int id) ;


long slmbx_count(unsigned int id) ;


long slmbx_send(unsigned int id , const unsigned char *msg , unsigned int len) ;


long slmbx_recv(unsigned int id , unsigned char *msg , unsigned int len) ;


long slmbx_length(unsigned int id) ;


int slmbx_dump() ;


#endif
