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
#include <unistd.h>
#include "skipList.h"

#define KEY_MAX_RANGE 4294967295 

// Necessary static 
static skipList *initList ;
unsigned int initialized ;

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
	// Default will always be one level
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

	// Only root user is allowed to call this function:
	if (getuid() != 0)
	{
		int errnum = EPERM ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(EPERM)) ;
		return -EPERM ;
	}
	
	// Checking to make sure mailbox system hasn't already been init
	if (initialized == 1)
	{
		int errnum = EALREADY ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EALREADY)) ;
		return -errnum ;
	}
	
	
	// Setting initialized = 1 so other functions can check for this 
	initialized = 1 ;
	
	// Primary invalid input handling
	// ptrs parameter must be non-zero
	if (ptrs == 0)
	{
		int errnum = EINVAL ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EINVAL)) ;		
		return -errnum ;
	}
	
	// Valid prob values are 2 , 4 , 8 , 16
	else if (prob == 2 || prob == 4 || prob == 8 || prob == 16)
	{
		if ((initList = (skipList *) malloc(sizeof(skipList))) == NULL)
		{
			int errnum = ENOMEM ;
			fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
			return -errnum ;
		}
		
		slNode *head ;
		
		if ((head = (slNode *) malloc(sizeof(struct slNode))) == NULL)
		{
			int errnum = ENOMEM ;
			fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
			return -errnum ;
		}
		
		initList -> head = head ;

		// Giving head key value greater than any legal id that can exist for a user (Ex: 2^32 - 1)
		head -> slNodeID = KEY_MAX_RANGE ;
		
		if ((head -> forwardList = (slNode **) malloc(sizeof(slNode*) * (ptrs + 1))) == NULL)
		{
			int errnum = ENOMEM ;
			fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
			return -errnum ;
		}
	
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
	return -errnum ;
}

// Useful for finding specific node
slNode *searchNode(unsigned int id)
{	
	slNode *foundNode = initList -> head ;
	
	int j ;
	for (j = initList -> level ; j >= 1 ; j--)
	{
		while (foundNode -> forwardList[j] -> slNodeID < id)
		{
			foundNode = foundNode -> forwardList[j] ;
		}
	}
	
	if (foundNode -> forwardList[1] -> slNodeID == id)
	{
		return foundNode -> forwardList[1] ;
	}
	else
	{
		return NULL ;
	}
	
	return NULL ;
}

long checkInit()
{
	// Check if mailbox has been initialized yet
	if (initialized != 1)
	{
		int errnum = ENODEV ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(ENODEV)) ;
		return 1 ;
	}
	return 0;
}

long checkPermissions(slNode *checkNode)
{
	// Seeing if user has permission to send to requested mailbox
	if (checkNode -> protect == 0)
	{
		if (checkNode -> ownedBy != getpid())
		{
			int errnum = EPERM ;
			fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(EPERM)) ;
			return 1 ;
		}
	}
	return 0 ;
}


/*
Shutdown the mailbox system, deleting all existing mailboxes and any messages cointained therein. Returns 0 on success, only root user allowed to call this function.
*/
long slmbx_shutdown(void)
{
	// Only root user is allowed to call this function:
	if (getuid() != 0)
	{
		int errnum = EPERM ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(EPERM)) ;
		return -errnum ;
	}
	
	if (checkInit() == 1)
		return -ENODEV ;
	
	initialized = 0 ;
	slNode *currNode = initList -> head -> forwardList[1] ;
	
	// Shutdown each node of the SL after destroying all mbxs within each node if exist
	while (currNode != initList -> head)
	{
		// Shutdown current node's mbx if it exists
		mailbox *current = currNode -> head ;
		mailbox *next ;
		
		while (current != NULL)
		{
			next = current -> next ;
			current -> message = 0 ;
			free(current) ;
			current = next ;
		}
		
		currNode -> head = NULL ;
			
		slNode *nextNode = currNode -> forwardList[1] ;
		free(currNode -> forwardList) ;
		free(currNode) ;
		currNode = nextNode ;
		
	}
	
	free(currNode -> forwardList) ;
	free(currNode) ;
	free(initList) ;
	
	return 0 ;
}


/*
Creates a new mailbox. Returns 0 on success and error on failure. 
*/
long slmbx_create(unsigned int id , int protected)
{
	if (checkInit() == 1) 
		return -ENODEV ;

	// If ID is 0 or (2^32 - 1) then its invalid ID and error returns for that create call
	if (id == 0 || id == KEY_MAX_RANGE)
	{
		int errnum = EINVAL ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EINVAL)) ;
		return -errnum ;
	}
	
	slNode *create[(initList -> maxLevel) + 1] ;
	slNode *newNode = initList -> head ;
	
	int j ;
	for (j = initList -> level ; j >= 1 ; j--)
	{
		while (newNode -> forwardList[j] -> slNodeID < id)
		{
			newNode = newNode -> forwardList[j] ;
		}
		
		create[j] = newNode ;
	}
	
	newNode = newNode -> forwardList[1] ;
	
	// User ID already exists case
	if (id == newNode -> slNodeID)
	{
		int errnum = EEXIST ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EEXIST))  ;
		return -errnum ;
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
		if ((newNode = (slNode *) malloc(sizeof(slNode))) == NULL)
		{
			int errnum = ENOMEM ;
			fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
			return -errnum ;
		}
		
		newNode -> slNodeID = id ;
		
		if ((newNode -> forwardList = (slNode **) malloc(sizeof(slNode *)* (newLevel + 1))) == NULL) 
		{
			int errnum = ENOMEM ;
			fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
			return -errnum ;
		}
		
		int i ;
		for (i = 1 ; i <= newLevel ; i++)
		{
			newNode -> forwardList[i] = create[i] -> forwardList[i] ;
			create[i] -> forwardList[i] = newNode ;
		}
		
		// Decide if mailbox only accesible to user who created the mailbox:
		if (protected != 0)
		{
			newNode -> protect = 0 ;
			newNode -> ownedBy = getpid() ;
		}
		
		// New mailbox attached to new node that was created for user:		
		// New mailbox attached, but it will be empty queue since no messages within!!
		newNode -> head = NULL ;
		newNode -> tail = NULL ;
	}
	return 0 ;
}


/*
Deletes specified mailbox if the user has permission to do so. Deletes all messages within inbox too. Returns 0 on success and error on failure. 
*/
long slmbx_destroy(unsigned int id)
{
	if (checkInit() == 1) 
		return -ENODEV ;

	slNode *removeUpdate[initList -> maxLevel + 1] ;
	slNode *removeNode = initList -> head ;

	int i ;
	for (i = initList -> level ; i >= 1 ; i--)
	{
		while (removeNode -> forwardList[i] -> slNodeID < id)
			{
				removeNode = removeNode -> forwardList[i] ;
			}
		removeUpdate[i] = removeNode ;
	}
	removeNode = removeNode -> forwardList[1] ;
	
	// Found the correct node user specified
	if (removeNode -> slNodeID == id)
	{
		if (checkPermissions(removeNode) == 1)
			return -EPERM ;
		
		// Destory mailbox/contained messages here before destroying the node itself
		mailbox *current = removeNode -> head ;
		mailbox *next ;
		
		while (current != NULL)
		{
			next = current -> next ;
			current -> message = 0 ;
			free(current) ;
			current = next ;
		}
		
		removeNode -> head = NULL ;
		
		int i ;
		for (i = 1 ; i <= initList -> level ; i++)
		{
			if (removeUpdate[i] -> forwardList[i] != removeNode)
			{
				break ;
			}
			
			removeUpdate[i] -> forwardList[i] = removeNode -> forwardList[i] ;
		}
		
		// Freeing (destroying) the requested slNodeID space
		if (removeNode)
		{
			free(removeNode -> forwardList) ;
			free(removeNode) ;
		}
		
		while(initList -> head -> forwardList[initList -> level] == initList -> head
			&& initList -> head )
		{
			initList -> level-- ;
		}
		
		// slNode successfully destroyed
		return 0 ;
	}
	
	// Error, couldn't find the user requested ID to destroy corresponding mailbox
	int errnum = ENOENT ;
	fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(ENOENT)) ;
	return -errnum ;
}


/*
Returns number of messages in the mailbox by id if it exists and they have permissions. Returns 0 on success and appropriate error code on failure. 
*/
long slmbx_count(unsigned int id) 
{
	if (checkInit() == 1)
		return -ENODEV ;
		
	slNode *findNode = searchNode(id) ;
		
	// Node couldn't be found
	if (findNode == NULL)
	{
		int errnum = ENOENT ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOENT)) ;
		return -errnum ;
	}
		
	// Mailbox is empty
	if (findNode -> head == NULL)
	{
		int errnum = ESRCH ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ESRCH)) ;
		return -errnum ;	
	}

	int messageCount = 0 ;
	
	mailbox *temp = findNode -> head ;
	while (temp != NULL)
	{
		messageCount++ ;
		temp = temp -> next ;
	}	
	
	return messageCount ;
}


/*
Sends a new message to the mailbox identified by id if it exists and the user can access it. Message will be read from user space ptr *msg and will be len bytes long. Returns 0 on success or appropriate error code on failure. 
*/
long slmbx_send(unsigned int id , const unsigned char *msg , unsigned int len) 
{
	if (checkInit() == 1) 
		return -ENODEV ;
		
	if (msg == NULL)
	{
		int errnum = EFAULT ;
		fprintf(stderr , "ERROR CODE %d: %s\n" , errnum , strerror(EFAULT)) ;
		return -errnum ;
	}
	
	slNode *findNode = searchNode(id) ;
		
	// Node couldn't be found
	if (findNode == NULL)
	{
		int errnum = ENOENT ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOENT)) ;
		return -errnum ;
	}

	if (checkPermissions(findNode) == 1)
		return -EPERM ;
	
	// At correct mailbox and user has correct permissions, can add their message to mailbox now
	char *transfer ;	
	mailbox *newMailbox ;
	
	if ((newMailbox = (struct mailbox*) malloc(sizeof(struct mailbox))) == NULL) 
	{
		int errnum = ENOMEM ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
		return -errnum ;
	}
	
	if ((transfer = (char *) malloc(len)) == NULL)
	{
		int errnum = ENOMEM ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
		return -errnum ;
	}
	
	memcpy(transfer , msg , len) ;
	
	newMailbox -> message = transfer ;
	newMailbox -> next = NULL ;
	
	if (findNode -> head == NULL)
	{
		findNode -> head = findNode -> tail = newMailbox ;
	}
	else
	{
		findNode -> tail -> next = newMailbox ;
		findNode -> tail = newMailbox ; 
	}
	
	return 0 ;
}


/*
Reads first message that is in mailbox identified by id if exists and user has access, storing either the entire length of the message or len bytes to user-space *msg, whichever is the lesser. 
*/
long slmbx_recv(unsigned int id , unsigned char *msg , unsigned int len) 
{
	if (checkInit() == 1) 
		return -ENODEV ;
	
	slNode *recvNode = searchNode(id) ;

	// Node couldn't be found
	if (recvNode == NULL)
	{
		int errnum = ENOENT ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOENT)) ;
		return -errnum ;
	}

	if (checkPermissions(recvNode) == 1)
		return -EPERM ;	
	
	// Mailbox is empty
	if (recvNode -> head == NULL)
	{
		int errnum = ESRCH ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ESRCH)) ;
		return -errnum ;	
	}
	else
	{
		size_t msgLen = strlen(recvNode -> head -> message) ;
		long byteLen ; 
			
		if (msgLen < len)
		{
			if ((msg = (char *) malloc(msgLen)) == NULL) 
			{
				int errnum = ENOMEM ;
				fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
				return -errnum ;
			}
			
			memcpy(msg , recvNode -> head -> message , msgLen) ;
			byteLen = msgLen ;
		}
		else
		{
			if ((msg = (char *) malloc(msgLen)) == NULL) 
			{
				int errnum = ENOMEM ;
				fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOMEM)) ;
				return -errnum ;
			}
			
			memcpy(msg , recvNode -> head -> message , len) ;
			byteLen = len ;
		}
		
		mailbox *temp = recvNode -> head ;
		recvNode -> head = recvNode -> head -> next ;
		temp -> message = 0 ;
		free(temp) ;
		
		return byteLen ;
	}
}


/*
Retrieves the length (in bytes) of the first message pending in the mailbox by id if it exists and user has access. Returns number of bytes in the first pending message in the mailbox on success, or error on fail. 
*/
long slmbx_length(unsigned int id) 
{
	if (checkInit() == 1)
		return -ENODEV ;
		
	slNode *lenNode = searchNode(id) ;
	
	// Node couldn't be found
	if (lenNode == NULL)
	{
		int errnum = ENOENT ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ENOENT)) ;
		return -errnum ;
	}
	
	if (checkPermissions(lenNode) == 1)
		return -EPERM ;
		
	// Mailbox is empty
	if (lenNode -> head == NULL)
	{
		int errnum = ESRCH ;
		fprintf(stderr , "ERROR CODE %d: %s\n", errnum , strerror(ESRCH)) ;
		return -errnum ;
	}
		
	// Mailbox exists and user has access to it
	size_t msgLen = strlen(lenNode -> head -> message) ;
		
	// Returning the number of bytes in the first pending message in specified mbx
	return msgLen ;
}

/*
Any needed contents that need dumping can be called here, for structure debugging purposes. 
*/
int slmbx_dump()
{
	if (checkInit() == 1) 
		return -ENODEV ;

	slNode *dumpNode = initList -> head ;	
	
	while (dumpNode && dumpNode -> forwardList[1] != initList -> head)
	{
		printf("slNodeID -> [%d] " , dumpNode -> forwardList[1] -> slNodeID) ;		

		if (dumpNode -> forwardList[1] -> head == NULL)
		{
			printf("\tMB -> [-EMPTY-]") ;		
		}
		else
		{
			mailbox *temp = dumpNode -> forwardList[1] -> head ;
			printf("\tMB -> ") ;
			while (temp != NULL)
			{
				printf("[ %s ]" , temp -> message) ;
				temp = temp -> next ;
			}		
		}

		printf("\n") ;
		dumpNode = dumpNode -> forwardList[1] ;		
	}	
	return 0 ;
}
