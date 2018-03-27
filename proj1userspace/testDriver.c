/*
testDriver.c
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.11.2018

Various testing of implementation of skipList.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "skipList.h"

#define KEY_MAX_RANGE 4294967295

// For testing!
int main()
{
	const unsigned char *msg = "testing!" , *msg2 = NULL , *msg3 = "" , *msg4 = "300" ;
	unsigned char *receiving ;
	unsigned char *new ;
	
	printf("\nTesting call before mailbox has been init:\n") ;
	slmbx_create(14 , 1) ;
	slmbx_destroy(100) ;
	slmbx_dump() ;
	
	printf("\nTesting shutdown before init:\n") ;
	slmbx_shutdown() ;
	
	slmbx_init(10 , 8) ;
	
	printf("\nAttempting to slmbx_init() twice:\n") ;
	slmbx_init(10 , 8) ;
	
	printf("\nCreating slNodes:\n") ;
	slmbx_create(100678 , 1) ;
	slmbx_create(14 , 1) ;
	slmbx_dump() ;
	
	printf("\nDeleting slNode 100678:\n") ;
	slmbx_destroy(100678) ;
	slmbx_dump() ;
	
	printf("\nTesting sending msg to invalid ID 420:\n") ;
	slmbx_send(420 , msg , 20) ;
	
	printf("\nTesting sending msgs (one being NULL):\n") ;
	
	slmbx_send(14 , msg2 , 17) ;
	slmbx_send(14 , msg3 , 20) ;
	slmbx_send(14 , msg4 , 20) ;
	slmbx_dump() ;
	
	printf("\nTesting recv from ID 14(4x w/ only 2 msgs and NULL):\n") ;
	slmbx_recv(14 , receiving , 15) ;
	slmbx_recv(14 , receiving , 15) ;
	slmbx_recv(14 , receiving , 15) ;
	slmbx_recv(14 , receiving , 15) ;
	slmbx_dump() ;

	printf("\nTesting slmbx_count() for ID 14 & 100678 (invalid):\n") ;
	slmbx_count(14) ;
	slmbx_count(100678) ;
	
	printf("\nTesting deleting slNode that doesn't exist:\n") ;
	slmbx_destroy(69);
	
	printf("\nTesting creating slNode that already exists:\n") ;
	slmbx_create(14, 16);

	printf("\nTesting invalid slNodeID creation:\n") ;
	slmbx_create(0, 4);
	slmbx_create(KEY_MAX_RANGE, 2);
	
	slmbx_shutdown() ;

	printf("\nTesting call after shutdown:\n") ;
	slmbx_dump() ;
	
	return 0 ;
}
