/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include <stdbool.h>
#include "../G8RTOS/G8RTOS.h"

#define MAX_NAME_LENGTH 16
/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

/* Create tcb struct here */
typedef struct tcb tcb_t;
typedef struct ptcb ptcb_t;



/*********************************************** Data Structure Definitions ***********************************************************/
struct tcb{
    int32_t* stackPointer;
    struct tcb* nextTcb;
    struct tcb* prevTcb;
    semaphore_t *blocked;
    bool asleep;
    uint32_t sleepCount;
    uint8_t priority;
    bool isAlive;
    threadId_t threadID;
    char threadName[MAX_NAME_LENGTH];

};

struct ptcb{
    struct ptcb* nextPtcb;
    struct ptcb* prevPtcb;
    void (*handler)(void);
    uint32_t period;
    uint32_t executionTime;
};
/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
