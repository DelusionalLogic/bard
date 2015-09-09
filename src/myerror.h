#ifndef MYERROR_H
#define MYERROR_H

// General Purpose errors
#define MYERR_ALLOCFAIL        1     //Mem Allocation fail
#define MYERR_NOSPACE          2     //Not enough space in buffer
#define MYERR_NOPROCESS        3     //Could not fork
#define MYERR_NOPERM           4     //Missing permission
#define MYERR_NOTINITIALIZED  10     //Object not initialized
#define MYERR_USERINPUTERR    50     //Error in user input

// Directory Errors
#define MYERR_NODIR          100     //No directory
#define MYERR_PATHLENGTH     101     //Path too long

// Container Errors
#define MYERR_OUTOFRANGE     200     //Index out of range 

// X Errors
#define MYERR_XOPEN          300     //Error while opening X component

//Unit Errors
#define MYERR_UNITWRONGTYPE  500     //Unit was of wrong type  

//WorkManager Errors
#define MYERR_BADFD          600     //The filedescriptor for a unit was invalid

#endif
