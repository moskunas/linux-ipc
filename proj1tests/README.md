**INSTRUCTIONS ON HOW TO RUN TEST PROGRAM:**

The test driver is just a single .c file named p1testDriver.c It tests the functionality of the required implementation as well as all of the various edge/error handling cases that are necessary for no crashes to occur in the kernel. The file is compiled with "make" and ran with "./p1testDriver". There are two other executables present in this directory (p1testDriver2 & p1testDriver3) that should not be ran. They are ran within p1testDriver with forks to test mailbox protection cases that you will see displayed so they need not be ran themselves. 

slmbx_init() and slmbx_shutdown() are in the same file as all my syscall testing so if you run p1testDriver while not root all the calls will just give the correct output of permission denied/no device exists. To see the test cases displayed you will of course have to run p1testDriver as root. 

**GENERAL STRATEGY FOR TESTING THE SYSTEM CALLS:**

The testing cases below include cases where there should be no error output (required functionality just being used normally) and intentional action to make sure program gives error outputs gracefully and doesnt crash the rest of the program execution. What is being tested for will be stated in userspace for each testing case segment before its tested. It will be visibly clear/stated if an error should be showing up or not. Brief expansion on the tests present in the driver are below:

1. The mailbox system can only be shutdown and initialized by the root user per the requirements. So when either of these are called by a non-root user the system won't be able to be initialized or shutdown and give an error. 

2. Called all of the system calls before slmbx_init() has been called to assure that all of them handle this situation properly. 

3. Called all of the system calls with legal parameters (things the syscalls are always supposed to be able to do properly without breaking the execution) and then dumped the data structure to show they worked properly. My implementation of slmbx_dump() displays any valid changes that were made to the system via dmesg. 

4. Called slmbx_send() with len value that will certainly make the call ultimately fail, displayed that it handles this fail gracefully and doesn't crash the kernel. The same situation can be handled with recv except the message won't be discarded like it is with send because the message still needs to exist in the mailbox system. 

5. Displayed parameter edge/invalid cases are taken care of. Called slmbx_init() after the mailbox system had already been initialized to assure that situation is handled properly. Tried to create mailboxes with IDs of the established invalid values of 0 and 2^32 - 1. Then called all applicable system calls with IDs that don't exist to show those cases get handled. 

6. Called any system calls that take pointers with a NULL pointer to assure they are handled gracefully and do not break the exeuction.

7. Called the system calls that return values to userspace to assure that they return the expected value given the current state of the mailbox system. 

8. Called slmbx_send() with len values that are too large to be handled in kernel space. This will make it fail so assure that the system call handles that situation and fails gracefully, not crashing the kernel.

9. Called all sytem calls from outside processes to test mailbox protection settings from outside process access. 
