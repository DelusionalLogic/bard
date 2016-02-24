#ifndef MYERROR_H
#define MYERROR_H

#define MYERR_BASE 2000

// General Purpose errors
#define MYERR_ALLOCFAIL       (MYERR_BASE+    1)     //Mem Allocation fail
#define MYERR_NOSPACE         (MYERR_BASE+    2)     //Not enough space in buffer
#define MYERR_NOPROCESS       (MYERR_BASE+    3)     //Could not fork
#define MYERR_NOPERM          (MYERR_BASE+    4)     //Missing permission
#define MYERR_NOTINITIALIZED  (MYERR_BASE+   10)     //Object not initialized
#define MYERR_USERINPUTERR    (MYERR_BASE+   50)     //Error in user input

// Directory Errors
#define MYERR_NODIR          (MYERR_BASE+   100)     //No directory
#define MYERR_PATHLENGTH     (MYERR_BASE+   101)     //Path too long

// Container Errors
#define MYERR_OUTOFRANGE     (MYERR_BASE+   200)     //Index out of range 

// X Errors
#define MYERR_XOPEN          (MYERR_BASE+   300)     //Error while opening X component

//Unit Errors
#define MYERR_UNITWRONGTYPE  (MYERR_BASE+   500)     //Unit was of wrong type  

//WorkManager Errors
#define MYERR_BADFD          (MYERR_BASE+   600)     //The filedescriptor for a unit was invalid

#endif
