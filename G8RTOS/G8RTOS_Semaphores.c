/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS/G8RTOS_CriticalSection.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <G8RTOS/G8RTOS_Structures.h>
#include <stdint.h>
#include "msp.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
	/* Implement this */
    int32_t sem = StartCriticalSection();
    *s = value;
    EndCriticalSection(sem);
}

/*
 * Waits for a semaphore to be available (value greater than 0)
 * 	- Decrements semaphore when available
 * 	- Spinlocks to wait for semaphore
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
	/* Implement this */
    int32_t sem = StartCriticalSection();
    (*s)--;

    if(*s < 0)
    {
        CurrentlyRunningThread->blocked = s;
        startContextSwitch();
    }
   EndCriticalSection(sem);

}

/*
 * Signals the completion of t....he usage of a semaphore
 * 	- Increments the semaphore value by 1
 * Param "s": Pointer to semaphore to be signalled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
	/* Implement this */
    int32_t sem = StartCriticalSection();

    (*s)++;

    if(*s <= 0)
    {
        tcb_t *pt = CurrentlyRunningThread->nextTcb;
        while(pt->blocked != s)
        {
            pt = pt->nextTcb;
        }

        pt->blocked = 0;
    }
    EndCriticalSection(sem);
}

/*********************************************** Public Functions *********************************************************************/


