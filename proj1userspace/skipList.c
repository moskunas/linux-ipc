/*
skipList.c
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.10.2018

Implementation for user-space skip list test implementation before porting into kernel space. (Will implement additional message queue specfication after skip list is working as intended as possible).
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "skipList.h"

#define KEY_MAX_RANGE 4294967295 

// Necessary static 
static skipList *initList ;


/* 
Code based from C11 standard to generate random numbers... 
*/
static unsigned int next_random = 9001 ;


static unsigned int generate_random_int(void)
{
	next_random = next_random * 1103515245 + 12345 ;
	return (next_random / 65536) % 32768 ;
}


static void seed_random(unsigned int seed)
{
	next_random = seed ;
}

/*
Function to give random level according to requirements (can't do floating point arith).
*/
static unsigned int randomLevel(void)
{
	// default will always be one level
	int level = 1 ;
	unsigned int num = generate_random_int() ; 
	
	// When promoteProb = 2:
	if (initList -> promoteProb == 2)
	{
		while (num >= 0 && num <= 16383)
		{
			level++ ;
			num = generate_random_int() ;
		}
	}
	
	// When promoteProb = 4:
	else if (initList -> promoteProb == 4)
	{
		while (num >= 0 && num <= 8191)
		{
			level++ ;
			num = generate_random_int() ;
		}
	}
	
	// When promoteProb = 8:
	else if (initList -> promoteProb == 8)
	{
		while (num >= 0 && num <= 4095)
		{
			level++ ;
			num = generate_random_int() ;
		}
	}
	
	// When promoteProb = 16:
	else
	{
		while (num >= 0 && num <= 2047)
		{
			level++ ;
			num = generate_random_int() ;
		}
	}
	
	return level ;
}


/*
Initializes the mailbox system, setting up the initial state of the skip list. Returns 0 on success, only root user allowed to call this function.
*/
long slmbx_init(unsigned int ptrs , unsigned int prob) 
{
	// Primary invalid input handling
	// ptrs parameter must be non-zero
	if (ptrs == 0)
	{
		int errnum = EINVAL ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EINVAL)) ;		
		return -1 ;
	}
	
	// valid prob values are 2 , 4 , 8 , 16
	else if (prob == 2 || prob == 4 || prob == 8 || prob == 16)
	{
		initList = (skipList *) malloc(sizeof(skipList)) ;
		slNode *head = (slNode *) malloc(sizeof(struct slNode)) ;
		initList -> head = head ;

		// Giving head key value greater than any legal id that can exist for a user (Ex: 2^32 - 1)
		head -> slNodeID = KEY_MAX_RANGE ;
		head -> forwardList = (slNode **) malloc(sizeof(slNode*) * (ptrs + 1)) ;
	
		int j ;
		for (j = 0 ; j <= ptrs ; j++)
		{
			head -> forwardList[j] = initList -> head ;
		}
	
		initList -> level = 1 ;	
		initList -> maxLevel = ptrs ;
		initList -> promoteProb = prob ;
		return 0 ;
	}

	// User entered invalid prob value
	int errnum = EINVAL ;
	fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EINVAL)) ;
	return -1 ;
}


/*
Shutdown the mailbox system, deleting all existing mailboxes and any messages cointained therein. Returns 0 on success, only root user allowed to call this function.
*/
long slmbx_shutdown(void)
{
	// will have to aid mailbox shutdown portion after creating SL shutdown portion?
	
	return 0 ;
}


/*
Creates a new mailbox. Returns 0 on success and error on failure. 

*/
long slmbx_create(unsigned int id , int protected)
{
	// If ID is 0 or (2^32 - 1) then its invalid ID and error returns for that create call
	if (id == 0 || id == KEY_MAX_RANGE)
	{
		int errnum = EINVAL ;
		fprintf(stderr , "ERROR CODE %d: %s (USER ID: %u) \n" , errnum , strerror(EINVAL) , id) ;
		return -1 ;
	}

	// have to create new column  
	// then a new mailbox connected to the new node created...
	// permissions will be applied to the mailbox once implemented after SL

	slNode *create[(initList -> maxLevel) + 1] ;
	slNode *newNode = initList -> head ;
	
	int i ;
	for (i = initList -> level ; i >= 1 ; i--)
	{
		while (newNode -> forwardList[i] -> slNodeID < id)
			{
				newNode = newNode -> forwardList[i] ;
			}
		create[i] = newNode ;
	}
	newNode = newNode -> forwardList[1] ;
	
	//User ID already exists case
	if (id == newNode -> slNodeID)
	{
		int errnum = EEXIST ;
		fprintf(stderr , "ERROR CODE %d: %s (USER ID: %d) \n" , errnum , strerror(EEXIST) , 
			newNode -> slNodeID)  ;
		return -1 ;
	}
	else
	{
		int newLevel ;
		newLevel = randomLevel() ;
		
		// If this new node level is higher than our current highest we need to update that
		if (newLevel > initList -> level)
		{
			int i ;
			for (i = initList -> level + 1 ; i <= newLevel ; i++)
			{
				create[i] = initList -> head ;
			}
			initList -> level = newLevel ;
		}
	
		// Now we allocate space for our new node that's being added
		newNode = (slNode *) malloc(sizeof(slNode)) ;
		newNode -> slNodeID = id ;
		newNode -> forwardList = (slNode **) malloc(sizeof(slNode *) * (newLevel + 1)) ;
		
		int i ;
		for (i = 1 ; i <= newLevel ; i++)
		{
			newNode -> forwardList[i] = create[i] -> forwardList[i] ;
			create[i] -> forwardList[i] = newNode ;
		}
	}
	return 0 ;
}


/*
Deletes specified mailbox if the user has permission to do so. Deletes all messages within inbox too. Returns 0 on success and error on failure. 
*/
long slmbx_destroy(unsigned int id)
{

	return 0 ;
}


/*
Returns number of messages in the mailbox by id if it exists and they have permissions. Returns 0 on success and appropriate error code on failure. 
*/
long slmbx_count(unsigned int id) 
{

	return 0 ;
}


/*
Sends a new message to the mailbox identified by id if it exists and the user can access it. Message will be read from user space ptr *msg and will be len bytes long. Returns 0 on success or appropriate error code on failure. 
*/
long slmbx_send(unsigned int id , const unsigned char *msg , unsigned int len) 
{

	return 0 ;
}


/*
Reads first message that is in mailbox identified by id if exists and user has access, storing either the entire length of the message or len bytes to user-space *msg, whichever is the lesser. 
*/
long slmbx_recv(unsigned int id , unsigned char *msg , unsigned int len) 
{

	return 0 ;
}


/*
Retrieves the length (in bytes) of the first message pending in the mailbox by id if it exists and user has access. Returns number of bytes in the first pending message in the mailbox on success, or error on fail. 
*/
long slmbx_length(unsigned int id) 
{

	return 0 ;
}

/*
Any needed contents that need dumping can be called here, for structure debugging purposes. 
*/
void slmbx_dump()
{
	slNode *dumpNode = initList -> head ;
	while (dumpNode && dumpNode -> forwardList[1] != initList -> head)
	{
		printf("slNodeID -> [%d]\n" , dumpNode -> forwardList[1] -> slNodeID) ;
		dumpNode = dumpNode -> forwardList[1] ;
	}	
}
