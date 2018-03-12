/*
testDriver.c
CMSC 421 UMBC 2018
Ian Moskunas / ianmosk1@umbc.edu
Project 1 
Started 3.11.2018

Testing implementation of skipList.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "skipList.h"

#define KEY_MAX_RANGE 4294967295

// For testing!
int main()
{
	unsigned int a = 6 , b = 8 ;
	slmbx_init(a , b) ;
	slmbx_create(0, b) ;
	slmbx_create(14, b) ;
	slmbx_create(1657, b) ;
	slmbx_create(9, b) ;
	slmbx_create(KEY_MAX_RANGE, b) ;
	slmbx_create(15, b) ;
	slmbx_create(69, b) ;
	slmbx_create(15, b) ;
	slmbx_dump() ;
	
	return 0 ;
}
