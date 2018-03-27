/*
p1testDriver.c
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.22.2018

Adequately testing my kernel changes to ensure they work under all situations (including error/edge cases).
*/

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <linux/kernel.h>
#include <sys/syscall.h>

#define KEY_MAX_RANGE 4294967295

#define __NR_slmbx_init		333
#define __NR_slmbx_shutdown	334
#define __NR_slmbx_create	335
#define __NR_slmbx_destroy	336
#define __NR_slmbx_count	337
#define __NR_slmbx_send		338
#define __NR_slmbx_recv		339
#define __NR_slmbx_length	340
#define __NR_slmbx_dump		341

long initSys(unsigned int ptrs , unsigned int prob)
{
	return syscall(__NR_slmbx_init , ptrs , prob) ;
}


long shutdownSys(void)
{
	return syscall(__NR_slmbx_shutdown) ;
}


long createSys(unsigned int id , int protected)
{
	return syscall(__NR_slmbx_create , id , protected) ;
}


long destroySys(unsigned int id)
{
	return syscall(__NR_slmbx_destroy , id) ;
}


long countSys(unsigned int id)
{
	return syscall(__NR_slmbx_count , id) ;
}


long sendSys(unsigned int id , const unsigned char *msg , unsigned int len)
{
	return syscall(__NR_slmbx_send , id , msg , len) ;
}


long recvSys(unsigned int id , unsigned char *msg , unsigned int len)
{
	return syscall(__NR_slmbx_recv , id , msg , len) ;
}


long lengthSys(unsigned int id)
{
	return syscall(__NR_slmbx_length , id) ;
}


int dumpSys(void)
{
	return syscall(__NR_slmbx_dump) ;
}


int main(int argc , char *argv[])
{
	unsigned char *msg = "test!!" , *msg2 = NULL , *msg3 = "" , *msg4 = "greetings" ;
	unsigned char *receiving = (unsigned char *) malloc(sizeof(unsigned char));
	unsigned char *receiving2 = (unsigned char *) malloc(sizeof(unsigned char));
	unsigned char *receivingNULL = NULL ;
	pid_t pid , waitpid ;
	char *argv_list[] = {"./p1testDriver2" , "./p1testDriver3" , NULL} ;
	
	printf("\n|----- PROJ1 TEST DRIVER ------|\n") ;		

	// Testing all syscalls before mailbox system initialized
	printf("\nTesting all syscalls before slmbx_init() called:\n----------------\n") ;

	if (createSys(15, 0) < 0)
		printf("slmbx_create() before slmbx_init(): %s\n" , strerror(errno)) ;

	if (destroySys(46) < 0)
		printf("slmbx_destroy() before slmbx_init(): %s\n" , strerror(errno)) ;

	if (countSys(800) < 0)
		printf("slmbx_count() before slmbx_init(): %s\n" , strerror(errno)) ;

	if (sendSys(913 , msg4 , 12) < 0)
		printf("slmbx_send() before slmbx_init(): %s\n" , strerror(errno)) ;

	if (recvSys(41 , receiving , 15) < 0)
		printf("slmbx_recv() before slmbx_init(): %s\n" , strerror(errno)) ;

	if (lengthSys(3) < 0)
		printf("slmbx_length() before slmbx_init(): %s\n" , strerror(errno)) ;

	if (shutdownSys() < 0)
		printf("slmbx_shutdown() before slmbx_init(): %s\n" , strerror(errno)) ;

	printf("----------------\n") ;

	// Testing doing all possible legal modifications to the mbxs, shouldn't error
	printf("\nTesting all legal syscall operations, 0 errors:\n----------------\n") ;	


	if (initSys(9 , 4) < 0)
		printf("INIT SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	
	if (createSys(478 , 0) < 0)
		printf("CREATE SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (sendSys(478 , msg3 , 10) < 0)
		printf("SEND SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (sendSys(478 , msg , 10) < 0)
		printf("SEND SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (sendSys(478 , msg , 10) < 0)
		printf("SEND SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (countSys(478) < 0)
		printf("COUNT SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (recvSys(478 , receiving , 9) < 0)
		printf("RECV SHOULDN'T ERROR: %s\n" , strerror(errno)) ;


	if (createSys(1281 , 1) < 0)
		printf("CREATE SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (destroySys(1281) < 0)
		printf("DESTROY SHOULDN'T ERROR: %s\n" , strerror(errno)) ;


	if (createSys(102 , 1) < 0)
		printf("CREATE SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (sendSys(102 , msg4 , 14) < 0)
		printf("SEND SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	if (lengthSys(102) < 0)
		printf("LENGTH SHOULDN'T ERROR: %s\n" , strerror(errno)) ;

	dumpSys() ; // Display changes in dmesg for visual validation if wanted

	printf("----------------\n") ;

	// Testing to ensure syscalls work under all required cases (errors & edges)
	printf("\nTesting all syscalls with invalid/nonexistent IDs:\n----------------\n") ;
	
	if (initSys(9 , 4) < 0)
		printf("slmbx_init() called while already initialized: %s\n" , strerror(errno)) ;

	if (createSys(KEY_MAX_RANGE, 1) < 0)
		printf("slmbx_create() with invalid ID 2^32 - 1: %s\n" , strerror(errno)) ;

	if (createSys(0, 1) < 0)
		printf("slmbx_create() with invalid ID 0: %s\n" , strerror(errno)) ;

	if (destroySys(22) < 0)
		printf("slmbx_destroy() with nonexistent ID 22: %s\n" , strerror(errno)) ;

	if (countSys(22) < 0)
		printf("slmbx_count() with nonexistent ID 22: %s\n" , strerror(errno)) ;

	if (sendSys(22 , msg4 , 12) < 0)
		printf("slmbx_send() with nonexistent ID 22: %s\n" , strerror(errno)) ;

	if (recvSys(22 , receiving , 15) < 0)
		printf("slmbx_recv() with nonexistent ID 22: %s\n" , strerror(errno)) ;

	if (lengthSys(22) < 0)
		printf("slmbx_length() with nonexistent ID 22: %s\n" , strerror(errno)) ;

	printf("----------------\n") ;

	printf("\nTesting handling of large len values:\n----------------\n") ;
	
	if (sendSys(478 , msg , 67108864) < 0)
		printf("slmbx_send() (userspace to kernel) with len value too large: %s\n" , 
			strerror(errno)) ;
	
	printf("----------------\n") ;

	// Testing NULL ptr handling
	printf("\nTesting all NULL pointer handling:\n----------------\n") ;
	
	if (sendSys(478 , msg2 , 14) < 0)
		printf("slmbx_send() given NULL pointer: %s\n" , strerror(errno)) ;
		
	if (recvSys(478 , receivingNULL , 14) < 0)
		printf("slmbx_recv() given NULL pointer: %s\n" , strerror(errno)) ;
		
	printf("----------------\n") ;
	
	// Testing correct return values from necessary syscalls
	printf("\nTesting for correct return values:\n----------------\n") ;
	
	printf("slmbx_count() called on mailbox with two messages: %ld\n" , countSys(478)) ;
	
	recvSys(478 , receiving2 , 2) ;
	printf("slmbx_recv() call should set msg to 'te': %s\n" , receiving2) ;
	
	printf("slmbx_length() call here should return 14: %ld\n" , lengthSys(102)) ;
	
	printf("slmbx_recv() call should return 9: %ld\n" , recvSys(102 , receiving , 9));
	
	printf("----------------\n") ;
	
	dumpSys() ; // Display changes inn dmesg for visual validation if wanted	
	
	printf("\nTesting calls on protected mailbox from diff process:\n----------------\n") ;
	
	pid = fork() ;
	if (pid == 0)
	{
		// Child
		execv("./p1testDriver2" , argv_list) ;
		printf("ERROR: Can't execute execv() command...\n") ;
		exit(1) ;
	}
	else if(pid < 0)
	{
		printf("ERROR: fork() failed...\n") ;
	}
	else
	{
		wait(NULL) ;
	}
	
	
	printf("----------------\n") ;

	printf("\nTesting calls on unprotected mailbox from diff process, 0 errors:\n----------------\n") ;
	
	pid = fork() ;
	if (pid == 0)
	{
		// Child
		execv("./p1testDriver3" , argv_list) ;
		printf("ERROR: Can't execute execv() command...\n") ;
		exit(1) ;
	}
	else if(pid < 0)
	{
		printf("ERROR: fork() failed...\n") ;
	}
	else
	{
		wait(NULL) ;
	}
	
	printf("----------------\n") ;		
	
	shutdownSys() ;
		
	return 0 ;
}
