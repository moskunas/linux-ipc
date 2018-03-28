/*
skipListK.c
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.20.2018

Kernel-space implementation of the userspace skipList.c program (rest in peace me)
*/

#include <linux/kernel.h>	
#include <linux/mutex.h>	// for lock/unlock
#include <linux/printk.h>	// printing
#include <linux/errno.h>	// error codes
#include <linux/string.h>	// memcpy(), etc.
#include <linux/types.h>	// size_t
#include <linux/slab.h>		// kmalloc()	
#include <linux/uaccess.h>	// User space copying
#include <linux/sched.h>	// getting pid 
#include <linux/cred.h>		// getting uid?

#include "skipListK.h"

#define KEY_MAX_RANGE 4294967295 

// Necessary static 
static skipList *initList ;
int initialized = 0 ;

// Necessary for locking
DEFINE_MUTEX(slMutex) ;

/* 
Code based from C11 standard to generate random numbers... 
*/
static unsigned int next_random = 9001 ;


static unsigned int generate_random_int(void)
{	
	next_random = next_random * 1103515245 + 12345 ;
	return (next_random / 65536) % 32768 ;
}


/*
Function to give random level according to requirements (can't do floating point arith).
*/
unsigned int randomLevel(void)
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


// See if user is allowed to access requested mailbox
int checkPermissions(slNode *checkNode)
{
	// Seeing if user has permission to send to requested mailbox
	if (checkNode -> protect == 0)
	{
		if (checkNode -> ownedBy != current -> pid)
		{
			return 1 ;
		}
	}	
	return 0 ;
}


/*
Initializes the mailbox system, setting up the initial state of the skip list. Returns 0 on success, only root user allowed to call this function.
*/
asmlinkage long slmbx_init(unsigned int ptrs , unsigned int prob) 
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;
	
	// Only root user is allowed to call this function:
	if (get_current_user() -> uid.val != 0)
	{
		mutex_unlock(&slMutex) ;
		return -EPERM ;
	}
	
	// Checking to make sure mailbox system hasn't already been init
	if (initialized != 0)
	{
		mutex_unlock(&slMutex) ;
		return -EALREADY ;
	}
	
	initialized = 1 ;

	slNode *head ;
	int j ;
	
	// Primary invalid input handling
	// ptrs parameter must be non-zero
	if (ptrs == 0)
	{
		mutex_unlock(&slMutex) ;
		return -EINVAL ;
	}
	
	// Valid prob values are 2 , 4 , 8 , 16
	else if (prob == 2 || prob == 4 || prob == 8 || prob == 16)
	{
		initList = kmalloc(sizeof(struct skipList) , GFP_KERNEL) ;
		if  (!initList)
		{
			kfree(initList) ;
			mutex_unlock(&slMutex) ;
			return -ENOMEM ;
		}
			
		head = kmalloc(sizeof(struct slNode) , GFP_KERNEL) ;
		if (!head)
		{
			kfree(head) ;
			mutex_unlock(&slMutex) ;
			return -ENOMEM ;
		}
		
		initList -> head = head ;

		// Giving head key value greater than any legal id that can exist for a user (Ex: 2^32 - 1)
		head -> slNodeID = KEY_MAX_RANGE ;
			
		head -> forwardList = kmalloc(sizeof(slNode*) * (ptrs + 1) , GFP_KERNEL) ;
		if (!(head -> forwardList))
		{
			kfree(head -> forwardList) ;
			mutex_unlock(&slMutex) ;
			return -ENOMEM ;
		}

		for (j = 0 ; j <= ptrs ; j++)
		{
			head -> forwardList[j] = initList -> head ;
		}
	
		initList -> level = 1 ;	
		initList -> maxLevel = ptrs ;
		initList -> promoteProb = prob ;
		
		mutex_unlock(&slMutex) ;
		return 0 ;
	}
	
	mutex_unlock(&slMutex) ;
	
	// User entered invalid prob value
	return -EINVAL ;
}


/*
Shutdown the mailbox system, deleting all existing mailboxes and any messages cointained therein. Returns 0 on success, only root user allowed to call this function.
*/
asmlinkage long slmbx_shutdown(void)
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;
		 
	if (initialized == 0)
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}
	
	initialized = 0 ;

	// Only root user is allowed to call this function:
	if (get_current_user() -> uid.val != 0)
	{
		mutex_unlock(&slMutex) ;
		return -EPERM ;
	}

	slNode *currNode , *nextNode ;
	mailbox *curr , *next ;
	
	currNode = initList -> head -> forwardList[1] ;
	
	// Shutdown each node of the SL after destroying all mbxs within each node if exist
	while (currNode != initList -> head)
	{
		// Shutdown current node's mbx if it exists
		curr = currNode -> head ;
		
		while (curr != NULL)
		{
			next = curr -> next ;
			curr -> message = 0 ;
			kfree(curr) ;
			curr = next ;
		}
		
		currNode -> head = NULL ;
			
		nextNode = currNode -> forwardList[1] ;
		kfree(currNode -> forwardList) ;
		kfree(currNode) ;
		currNode = nextNode ;
		
	}
	
	kfree(currNode -> forwardList) ;
	kfree(currNode) ;
	kfree(initList) ;
		
	mutex_unlock(&slMutex) ;
	return 0 ;
}


/*
Creates a new mailbox. Returns 0 on success and error on failure. 
*/
asmlinkage long slmbx_create(unsigned int id , int protected)
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}

	// If ID is 0 or (2^32 - 1) then its invalid ID and error returns for that create call
	if (id == 0 || id == KEY_MAX_RANGE)
	{
		mutex_unlock(&slMutex) ;
		return -EINVAL ;
	}

	slNode *create[(initList -> maxLevel) + 1] ;
	slNode *newNode ;
	int j = 0 , i = 0 ;
	unsigned int newLevel ;
	
	newNode = initList -> head ;
	
	
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
		mutex_unlock(&slMutex) ;
		return -EEXIST;
	}
	else
	{
		newLevel = randomLevel() ;
		
		// If this new node level is higher than our current highest we need to update that
		if (newLevel > initList -> level)
		{
			for (i = initList -> level + 1 ; i <= newLevel ; i++)
			{
				create[i] = initList -> head ;
			}
			initList -> level = newLevel ;
		}
		
	
		// Now we allocate space for our new node that's being added
		newNode = kmalloc(sizeof(struct slNode) , GFP_KERNEL) ;
		if (!newNode)
		{
			kfree(newNode) ;
			mutex_unlock(&slMutex) ;
			return -ENOMEM ;
		}
		
		newNode -> slNodeID = id ;


		newNode -> forwardList = kmalloc(sizeof(slNode *) * (newLevel + 1) , GFP_KERNEL) ;
		if (!(newNode -> forwardList))
		{	
			kfree(newNode -> forwardList) ;
			mutex_unlock(&slMutex) ;
			return -ENOMEM ;
		}
		
		
		for (i = 1 ; i <= newLevel ; i++)
		{
			newNode -> forwardList[i] = create[i] -> forwardList[i] ;
			create[i] -> forwardList[i] = newNode ;
		}
		
		
		// Decide if mailbox only accesible to user who created the mailbox:
		if (protected != 0)
		{
			newNode -> protect = 0 ;
			newNode -> ownedBy = current -> pid ;
		}
		else
		{
			newNode -> protect = 1 ;
		}
		
		// New mailbox attached to new node that was created for user:		
		// New mailbox attached, but it will be empty queue since no messages within!!
		newNode -> head = NULL ;
		newNode -> tail = NULL ;
	}
	
	mutex_unlock(&slMutex) ;
	return 0 ;
}


/*
Deletes specified mailbox if the user has permission to do so. Deletes all messages within inbox too. Returns 0 on success and error on failure. 
*/
asmlinkage long slmbx_destroy(unsigned int id)
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}

	slNode *removeUpdate[(initList -> maxLevel) + 1] ;
	slNode *removeNode ;
	mailbox *curr , *next ;
	int i = 0 ;

	removeNode = initList -> head ;

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
		{
			mutex_unlock(&slMutex) ;
			return -EPERM ;
		}
		
		// Destory mailbox/contained messages here before destroying the node itself
		curr = removeNode -> head ;
		
		while (curr != NULL)
		{
			next = curr -> next ;
			curr -> message = 0 ;
			kfree(curr) ;
			curr = next ;
		}
		
		removeNode -> head = NULL ;
		
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
			kfree(removeNode -> forwardList) ;
			kfree(removeNode) ;
		}
		
		while (initList -> head == initList -> head -> forwardList[initList -> level]
			&& initList -> level > 1) ;
		{
			initList -> level-- ;
		}
		
		mutex_unlock(&slMutex) ;
		
		// slNode successfully destroyed
		return 0 ;
	}
	
	mutex_unlock(&slMutex) ;
	
	// Error, couldn't find the user requested ID to destroy corresponding mailbox
	return -ENOENT;
}


/*
Returns number of messages in the mailbox by id if it exists and they have permissions. Returns 0 on success and appropriate error code on failure. 
*/
asmlinkage long slmbx_count(unsigned int id) 
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}

	slNode *findNode ;
	int messageCount ;
	mailbox *temp ;
		
	findNode = searchNode(id) ;
		
	// Node couldn't be found
	if (findNode == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ENOENT ;
	}
	
	if (checkPermissions(findNode) == 1)
	{
		mutex_unlock(&slMutex) ;
		return -EPERM ;
	}
			
	// Mailbox is empty
	if (findNode -> head == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ESRCH ;	
	}
	
	messageCount = 0 ;
	
	temp = findNode -> head ;
	while (temp != NULL)
	{
		messageCount++ ;
		temp = temp -> next ;
	}	
	
	mutex_unlock(&slMutex) ;
	return messageCount ;
}


/*
Sends a new message to the mailbox identified by id if it exists and the user can access it. Message will be read from user space ptr *msg and will be len bytes long. Returns 0 on success or appropriate error code on failure. 
*/
asmlinkage long slmbx_send(unsigned int id , const unsigned char *msg , unsigned int len) 
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}	
			
	if (msg == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -EFAULT ;
	}
	
	mailbox *newMailbox ;
	slNode *findNode ;
	
	findNode = searchNode(id) ;
		
	// Node couldn't be found
	if (findNode == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ENOENT ;
	}

	if (checkPermissions(findNode) == 1)
	{
		mutex_unlock(&slMutex) ;
		return -EPERM ;
	}
	
	// At correct mailbox and user has correct permissions, can add their message to mailbox now
	newMailbox = kmalloc(sizeof(struct mailbox) , GFP_KERNEL) ;
	if (!newMailbox)
	{
		kfree(newMailbox) ;
		mutex_unlock(&slMutex) ;
		return -ENOMEM ;
	}

	newMailbox -> message = kmalloc(len, GFP_KERNEL) ;
	if (!(newMailbox -> message))
	{
		kfree(newMailbox -> message) ;
		mutex_unlock(&slMutex) ;
		return -ENOMEM ;
	}
	
	if (copy_from_user(newMailbox -> message , msg , len)) 
	{
		kfree(newMailbox -> message) ;
		mutex_unlock(&slMutex) ;
		return -ENOMEM ;
	}
	
	newMailbox -> messageLen = len ;
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
	
	mutex_unlock(&slMutex) ;
	return 0 ;
}


/*
Reads first message that is in mailbox identified by id if exists and user has access, storing either the entire length of the message or len bytes to user-space *msg, whichever is the lesser. 
*/
asmlinkage long slmbx_recv(unsigned int id , unsigned char *msg , unsigned int len) 
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}
	
	if (msg == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -EFAULT ;
	}

	mailbox *temp ;
	slNode *recvNode ;
	unsigned int byteLen ;
	
	recvNode = searchNode(id) ;

	// Node couldn't be found
	if (recvNode == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ENOENT ;
	}

	if (checkPermissions(recvNode) == 1)
	{
		mutex_unlock(&slMutex) ;
		return -EPERM ;	
	}
	
	// Mailbox is empty
	if (recvNode -> head == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ESRCH;	
	}
	else
	{ 
			
		if (recvNode -> head -> messageLen < len)
		{
			if (copy_to_user(msg , recvNode -> head -> message , 
				recvNode -> head -> messageLen)) 
			{
				mutex_unlock(&slMutex) ;
				return -ENOMEM ;
			}
			
			byteLen = recvNode -> head -> messageLen ;
		}
		else
		{
			if (copy_to_user(msg , recvNode -> head -> message , len)) 
			{
				mutex_unlock(&slMutex) ;
				return -ENOMEM ;
			}
			
			byteLen = len ;
		}
		
		temp = recvNode -> head ;
		recvNode -> head = recvNode -> head -> next ;
		temp -> message = 0 ;
		kfree(temp) ;
		
		mutex_unlock(&slMutex) ;
		return byteLen ;
	}
}


/*
Retrieves the length (in bytes) of the first message pending in the mailbox by id if it exists and user has access. Returns number of bytes in the first pending message in the mailbox on success, or error on fail. 
*/
asmlinkage long slmbx_length(unsigned int id) 
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}

	slNode *lenNode ;		
	lenNode = searchNode(id) ;
	
	// Node couldn't be found
	if (lenNode == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ENOENT ;
	}
	
	if (checkPermissions(lenNode) == 1)
	{
		mutex_unlock(&slMutex) ;
		return -EPERM ;
	}
		
	// Mailbox is empty
	if (lenNode -> head == NULL)
	{
		mutex_unlock(&slMutex) ;
		return -ESRCH ;
	}
				
	mutex_unlock(&slMutex) ;

	// Returning the number of bytes in the first pending message in specified mbx
	return lenNode -> head -> messageLen ;
}

/*
Any needed contents that need dumping can be called here, for structure debugging purposes. 
*/
asmlinkage int slmbx_dump(void)
{
	if (mutex_lock_interruptible(&slMutex) < 0)
		 return -EINTR ;

	if (initialized == 0) 
	{
		mutex_unlock(&slMutex) ;
		return -ENODEV ;
	}

	slNode *dumpNode ;
	mailbox *temp ;

	dumpNode = initList -> head ;	
	
	printk("V---------------- SL MBX DUMP ----------------V") ;	

	while (dumpNode && dumpNode -> forwardList[1] != initList -> head)
	{
		printk("slNodeID -> [%u] " , dumpNode -> forwardList[1] -> slNodeID) ;		

		if (dumpNode -> forwardList[1] -> head == NULL)
		{
			printk("\tMB -> [-EMPTY-]") ;		
		}
		else
		{
			temp = dumpNode -> forwardList[1] -> head ;
			printk("\tMB -> ") ;
			while (temp != NULL)
			{
				printk("[ %s ]" , temp -> message) ;
				temp = temp -> next ;
			}		
		}

		printk("\n") ;
		dumpNode = dumpNode -> forwardList[1] ;		
	}	
	
	mutex_unlock(&slMutex) ;
	return 0 ;
}
