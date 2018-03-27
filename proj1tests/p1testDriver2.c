/*
p1testDriver2.c
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.26.2018

Adequately testing my kernel changes to ensure they work under all situations (including error/edge cases).
*/

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
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

	if (sendSys(102, msg , 15) < 0)
		printf("slmbx_send() to protected mailbox: %s\n" , strerror(errno)) ;
	
	if (countSys(102) < 0)
		printf("slmbx_count() to protected mailbox: %s\n" , strerror(errno)) ;
	
	if (recvSys(102, receiving , 15) < 0)
		printf("slmbx_recv() to protected mailbox: %s\n" , strerror(errno)) ;
	
	if (lengthSys(102) < 0)
		printf("slmbx_length() to protected mailbox: %s\n" , strerror(errno)) ;
	
	if (destroySys(102) < 0)
		printf("slmbx_destroy() to protected mailbox: %s\n" , strerror(errno)) ;
	
	
	return 0 ;		
}
