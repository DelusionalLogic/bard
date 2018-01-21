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

#include <stdbool.h>

void error_init();
void error_new(char* file, int line, char* format, ...);
void error_append(char* file, int line, char* format, ...);

bool error_waiting();

void error_eat();
void error_print();
void error_abort() __attribute__((__noreturn__));

#define THROW_NEW(ret_val, format, ...) do{ error_new(__FILE__, __LINE__, format, ##__VA_ARGS__); return ret_val; } while(0)
#define THROW_CONT(ret_val, format, ...) do{ error_append(__FILE__, __LINE__, format, ##__VA_ARGS__); return ret_val; } while(0)

#define VTHROW_NEW(format, ...) do{ error_new(__FILE__, __LINE__, format, ##__VA_ARGS__); return; } while(0)
#define VTHROW_CONT(format, ...) do{ error_append(__FILE__, __LINE__, format, ##__VA_ARGS__); return; } while(0)

#define ERROR_NEW(format, ...) error_new(__FILE__, __LINE__, format, ##__VA_ARGS__)
#define ERROR_CONT(format, ...) error_append(__FILE__, __LINE__, format, ##__VA_ARGS__)

#define ERROR_ABORT(format, ...) do{ if(error_waiting()) { ERROR_CONT(format, ##__VA_ARGS__); error_abort();} }while(0)

#define PROP_THROW(ret_val, format, ...) do{ if(error_waiting()) THROW_CONT(ret_val, format, ##__VA_ARGS__); }while(0)
#define VPROP_THROW(format, ...) do{ if(error_waiting()) VTHROW_CONT(format, ##__VA_ARGS__); }while(0)
#endif
