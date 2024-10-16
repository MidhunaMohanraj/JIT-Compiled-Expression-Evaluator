/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * jitc.c
 */

/**
 * Needs:
 *   fork()
 *   execv()
 *   waitpid()
 *   WIFEXITED()
 *   WEXITSTATUS()
 *   dlopen()
 *   dlclose()
 *   dlsym()
 */

/* research the above Needed API and design accordingly */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include "system.h"
#include "jitc.h"

struct jitc {
    void *outLibHandle;
};

/**
 * Compiles a C program into a dynamically loadable module.
 *
 * input : the file pathname of the C program
 * output: the file pathname of the dynamically loadable module
 *
 * return: 0 on success, otherwise error
 */
int jitc_compile(const char *input, const char *output) {
    
    pid_t processId;
    char* GCC_PATH = "/usr/bin/gcc";  

    char* args[7];
    args[0] = "gcc";           /* program name */
    args[1] = "-shared";       /* instructs gcc to create a .so file */ 
    args[2] = "-o";            /* specifies next file will be out file*/
    args[3] = (char *) output; /* name of output file (binary code will be generated here) */
    args[4] = "-fPIC";         /* gives every function a full address */
    args[5] = (char *) input;  /* input file that will be compiled */
    args[6] = NULL;            /* required & terminates args list */
    
    /* create the fork*/
    processId  = fork();

    /* Fork was unsucessful */
    if(processId < 0) {
        printf("ERROR - fork failed");
        return -1;
    }

    /* Fork created sucessfully, now child processes run */ 
    if(processId == 0) {
        /* child processes: creates the .so file and puts it in storage */
        /* .so file = a binary file that is dynamically loaded by a program during run time */
        /* ececv() replaces the current process image with a new process image (gcc)*/
        if(execv(GCC_PATH, args) == -1) {
            printf("ERROR: execv failed\n");
            return -1;
        }
    }

    /* parent runs, but must wait for child to finish first */
    else {
        int status; /* used to store the exit code of the child process */

        /* waitpid: needs the pId to wait for, and the address of the exit code of the process */
        /*          returns the child process Id*/
        pid_t child_processId = waitpid(processId, &status, 0);
        if(child_processId < 0) {
            printf("ERROR - waitpid failed\n");
            return -1;
        }

        /* successfully waited for child */
        /* WIFEXITED returns true (1) if child process exited/returned normally */
        /* WEXITSTATUS checks that the child process completed sucessfulyl */
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            /* GCC was sucessful -> now return 0 */ 
            return 0;
        } else {
            printf("ERROR - Child process ended unexpectedly -> GCC did not work\n");
            printf("\tExit status: %d\n", WEXITSTATUS(status));
            return -1;
        }
    }

    return 0;
}


/**
 * Loads a dynamically loadable module into the calling process' memory for
 * execution because the .so file is currently somewhere on the disk unknown to this program.
 *
 * pathname: the file pathname of the dynamically loadable module
 *
 * return: an opaque handle (reference to the loaded library) or NULL on error
 */

struct jitc *jitc_open(const char *pathname) {
    struct jitc *jitc;

    /* dynamically allocate a block of memeory for jitc */
    if(!(jitc = malloc(sizeof(*jitc)))) {
        TRACE("out of memory for jitc");
        return NULL;
    }

    /* call dlopen(): - dlopen loads the module into memory and returns a reference to the
                      loaded library that can be used later 
                      - Returns NULL if error
                      - RTLD_LAZZY: functions are not resolved until they are first used */
    jitc->outLibHandle = dlopen(pathname, RTLD_LAZY);
    if(!jitc->outLibHandle){
        printf("ERROR - No loadable module");
        
        /* deallocate memory */
        free(jitc); 
        return NULL;
    }
    
    return jitc;

}


/**
 * Unloads a previously loaded dynamically loadable module.
 *
 * jitc: an opaque handle previously obtained by calling jitc_open()
 *
 * Note: jitc may be NULL
 */

void jitc_close(struct jitc *jitc) {
    /* dlclose:  - removes library from memory 
                 - frees any resources being used by lib 
                 - returns 0 on sucess */
    if(dlclose(jitc->outLibHandle)) {
        printf("ERROR - dlclose error");
    }

    /* deallocate memory */
    free(jitc);
}

/**
 * Searches for a symbol (function) in the dynamically loaded module associated with jitc.
 *
 * jitc: an opaque handle previously obtained by calling jitc_open()
 *
 * return: the memory address of the start of the symbol, or 0 on error
 */
long jitc_lookup(struct jitc *jitc, const char *symbol) {
    void* address; /* saves the return of dlsym, which is a void pointer */

    /* assigns address to the memory address of the function we want to call (symbol)*/
    address = dlsym(jitc->outLibHandle, symbol);

    /* dlsym returns NULL if symbol is not found */
    if(!address) {
        /* returns 0 on error */
        return 0;
    }

    /* cast the void * to a long to match return */
    return (long) address;
}