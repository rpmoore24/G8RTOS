/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS/G8RTOS_Scheduler.h>
#include <G8RTOS/G8RTOS_Structures.h>
#include <stdint.h>
#include "msp.h"
#include "BSP.h"
#include "Game.h"


/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;


/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

//Period Thread Control Blocks
static ptcb_t periodicThreadControlBlocks[MAX_PERIODIC_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];


/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;
static uint32_t NumberOfPeriodicThreads;
static uint16_t IDCounter = 0;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
	/* Implement this */
    SysTick_Config(numCycles);
}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 */
void G8RTOS_Scheduler()
{
    tcb_t* tempNextThread = CurrentlyRunningThread->nextTcb;
    uint16_t currentMaxPriority = 256;
	/* Implement This */
    for(int i = 0; i < NumberOfThreads; i++)
    {
        if(tempNextThread->blocked == 0 && tempNextThread->asleep == false)
        {
            if(tempNextThread->priority <= currentMaxPriority)
            {
                CurrentlyRunningThread = tempNextThread;
                currentMaxPriority = tempNextThread->priority;
                tempNextThread = tempNextThread->nextTcb;
            }
            else
                tempNextThread = tempNextThread->nextTcb;
        }
        else
            tempNextThread = tempNextThread->nextTcb;
    }
}

void startContextSwitch()
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


/*
 * SysTick Handler
 * Currently the Systick Handler will only increment the system time
 * and set the PendSV flag to start the scheduler
 *
 * In the future, this function will also be responsible for sleeping threads and periodic threads
 */
void SysTick_Handler()
{
	/* Implement this */
    SystemTime++;
    if(SystemTime == 1001)
    {
        int y;
        y++;
    }
    ptcb_t* ppt = &periodicThreadControlBlocks[0];
    //Check for periodic threads
    for(int i = 0; i < NumberOfPeriodicThreads; i++)
    {
        if(SystemTime == ppt->executionTime)
        {
            ppt->executionTime = SystemTime + ppt->period;
            ppt->handler();
        }
        ppt = ppt->nextPtcb;
    }

    //Iterate through threads and check if any need to be woken up
    tcb_t* pt = CurrentlyRunningThread->nextTcb;
    for(int i = 0; i < NumberOfThreads -1; i++)
    {
        if(pt->asleep && pt->sleepCount == SystemTime)
        {
            pt->asleep = false;
            pt->sleepCount = 0;
        }
        pt = pt->nextTcb;
    }

    //Trigger PendSV for context switch
    startContextSwitch();
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
	/* Implement this */
    //Set system time to 0
    SystemTime = 0;
    //Set number of threads to 0
    NumberOfThreads = 0;
    NumberOfPeriodicThreads = 0;

    //move vtor table
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4); // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;

    //Initialize all hardware on board
    BSP_InitBoard();
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
	/* Implement this */
    CurrentlyRunningThread = &threadControlBlocks[0];
    G8RTOS_Scheduler();

    //initlaize systick
    __NVIC_SetPriority(PendSV_IRQn, 0xff);
    __NVIC_SetPriority(SysTick_IRQn, 0xff);
    InitSysTick(ClockSys_GetSysFreq()/1000);
    G8RTOS_Start();

    return -1;
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
int G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char* name)
{
	/* Implement this */
    int32_t criticalSection = StartCriticalSection();

    if(NumberOfThreads < MAX_THREADS)
    {
        int16_t threadIndex = -1;
        for(int i = 0; i < MAX_THREADS; i++)
        {
            if (!threadControlBlocks[i].isAlive)
            {
                threadIndex = i;
                break;
            }
        }

        if(threadIndex == -1)
        {
            EndCriticalSection(criticalSection);
            return -1;
        }

        //Make next pointer of new tcb point to start of list
        if(NumberOfThreads == 0)
        {
           //If first thread, point to itself
           threadControlBlocks[threadIndex].nextTcb = &threadControlBlocks[threadIndex];
           threadControlBlocks[threadIndex].prevTcb = &threadControlBlocks[threadIndex];
        }
        else
        {
            threadControlBlocks[threadIndex].nextTcb = CurrentlyRunningThread;
            threadControlBlocks[threadIndex].prevTcb = CurrentlyRunningThread->prevTcb;
            CurrentlyRunningThread->prevTcb = &threadControlBlocks[threadIndex];
            threadControlBlocks[threadIndex].prevTcb->nextTcb = &threadControlBlocks[threadIndex];
        }

        threadControlBlocks[threadIndex].asleep = false;
        threadControlBlocks[threadIndex].sleepCount = 0;
        threadControlBlocks[threadIndex].blocked = 0;
        threadControlBlocks[threadIndex].priority = priority;
        threadControlBlocks[threadIndex].threadID = threadIndex;
        threadControlBlocks[threadIndex].isAlive = true;
        strcpy(threadControlBlocks[threadIndex].threadName, name);


        threadControlBlocks[threadIndex].stackPointer = &threadStacks[threadIndex][STACKSIZE-1];

        //Initialize stack with dummy values in order
        //Sets PSR to have thumb bit set
        *(threadControlBlocks[threadIndex].stackPointer--) = THUMBBIT;
        //Sets PC to thread function
        *(threadControlBlocks[threadIndex].stackPointer--) = (int32_t) threadToAdd;
        //Sets LR
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        //Sets R12
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        //Sets R3-R0
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        //Sets R11-R4
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer--) = threadIndex;
        *(threadControlBlocks[threadIndex].stackPointer) = threadIndex;

        NumberOfThreads++;
    }
    else
    {
        EndCriticalSection(criticalSection);
        return -1;
    }

    EndCriticalSection(criticalSection);
    return 1;
}

threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->threadID;
}

sched_ErrCode_t G8RTOS_KillThread(threadId_t threadId)
{
    int32_t criticalSection = StartCriticalSection();

    //Return if there's only one thread running
    if(NumberOfThreads == 1)
    {
        EndCriticalSection(criticalSection);
        return CANNOT_KILL_LAST_THREAD;
    }

    tcb_t* nextThread = CurrentlyRunningThread->nextTcb;
    for(int i = 0; i < NumberOfThreads - 1; i++)
    {
        if(nextThread->threadID == threadId)
        {
            break;
        }
        nextThread = nextThread->nextTcb;
    }

    //If tempNextThread is the CRT, then the threadId was not found anywhere else
    if(nextThread == CurrentlyRunningThread)
    {
        //Check if the CRT is to be killed
        if(CurrentlyRunningThread->threadID == threadId)
        {
            //Kill the thread and update the pointers
            CurrentlyRunningThread->isAlive = false;
            CurrentlyRunningThread->nextTcb->prevTcb = CurrentlyRunningThread->prevTcb;
            CurrentlyRunningThread->prevTcb->nextTcb = CurrentlyRunningThread->nextTcb;
            //If this thread is using/waiting on a resource, free it
            if(CurrentlyRunningThread->blocked != 0)
            {
                *CurrentlyRunningThread->blocked += 1;
            }
            //Start a context switch once this function exits
            startContextSwitch();
        }
        else
        {
            //threadId was not found anywhere
            EndCriticalSection(criticalSection);
            return THREAD_DOES_NOT_EXIST;
        }
    }
    else
    {
        //Kill the thread and update the pointers
        nextThread->isAlive = false;
        nextThread->nextTcb->prevTcb = nextThread->prevTcb;
        nextThread->prevTcb->nextTcb = nextThread->nextTcb;
        //If this thread is using/waiting on a resource, free it
        if(nextThread->blocked != 0)
        {
            *nextThread->blocked += 1;
        }
    }

    //Decrement the number of threads
    NumberOfThreads--;

    EndCriticalSection(criticalSection);
    return NO_ERROR;
}

sched_ErrCode_t G8RTOS_KillSelf()
{
    //Enter a critical section
    int32_t criticalSection = StartCriticalSection();

    //Return if there's only one thread running
    if(NumberOfThreads == 1)
    {
        EndCriticalSection(criticalSection);
        return CANNOT_KILL_LAST_THREAD;
    }

    //Kill the thread and update the pointers
    CurrentlyRunningThread->isAlive = false;
    CurrentlyRunningThread->nextTcb->prevTcb = CurrentlyRunningThread->prevTcb;
    CurrentlyRunningThread->prevTcb->nextTcb = CurrentlyRunningThread->nextTcb;
    //If this thread is using/waiting on a resource, free it
    if(CurrentlyRunningThread->blocked != 0)
    {
        *CurrentlyRunningThread->blocked += 1;
    }
    //Start a context switch once this function exits
    startContextSwitch();

    //Decrement the number of threads
    NumberOfThreads--;

    EndCriticalSection(criticalSection);
    return NO_ERROR;

}


int32_t G8RTOS_AddPeriodicThread(void (*threadToAdd)(void), uint32_t period, uint32_t offset)
{
    int32_t criticalSection = StartCriticalSection();
    if(NumberOfPeriodicThreads < MAX_PERIODIC_THREADS)
    {
        //Make new ptcb next pointer point at first ptcb
        periodicThreadControlBlocks[NumberOfPeriodicThreads].nextPtcb = &periodicThreadControlBlocks[0];

        if(NumberOfPeriodicThreads == 0)
        {
            //If only ptcb, make prev pointer point at itself
            periodicThreadControlBlocks[NumberOfPeriodicThreads].prevPtcb = &periodicThreadControlBlocks[0];
        }
        else
        {
            //Break chain and place new ptcb in chain
            periodicThreadControlBlocks[NumberOfPeriodicThreads].prevPtcb = &periodicThreadControlBlocks[NumberOfPeriodicThreads-1];
            periodicThreadControlBlocks[NumberOfPeriodicThreads-1].nextPtcb = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
            periodicThreadControlBlocks[0].prevPtcb = &periodicThreadControlBlocks[NumberOfPeriodicThreads];
        }

        periodicThreadControlBlocks[NumberOfPeriodicThreads].handler = threadToAdd;
        periodicThreadControlBlocks[NumberOfPeriodicThreads].period = period;
        periodicThreadControlBlocks[NumberOfPeriodicThreads].executionTime = SystemTime + period + offset;
        NumberOfPeriodicThreads++;
    }
    else
    {
        EndCriticalSection(criticalSection);
        return -1;
    }

    EndCriticalSection(criticalSection);
    return 1;
}

sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn)
{
    int32_t criticalSection = StartCriticalSection();
    if(IRQn < PSS_IRQn || IRQn > PORT6_IRQn)
    {
        return IRQn_INVALID;
    }
    if(priority > 6)
    {
        EndCriticalSection(criticalSection);
        return HWI_PRIORITY_INVALID;
    }
    __NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);
    __NVIC_SetPriority(IRQn, (uint32_t)priority);
    NVIC_EnableIRQ(IRQn);
    EndCriticalSection(criticalSection);
    return NO_ERROR;
}

void OS_Sleep(uint32_t duration)
{
    CurrentlyRunningThread->sleepCount = duration + SystemTime;
    CurrentlyRunningThread->asleep = true;
    startContextSwitch();
}

sched_ErrCode_t G8RTOS_KillAllThreads()
{
    //Enter a critical section
    int32_t criticalSection = StartCriticalSection();

    //Return if there's only one thread running
    if(NumberOfThreads == 1)
    {
        EndCriticalSection(criticalSection);
        return CANNOT_KILL_LAST_THREAD;
    }

    tcb_t * nextThread = CurrentlyRunningThread->nextTcb;
    //Iterate through the whole list of threads and kill all of them
    while(nextThread != CurrentlyRunningThread)
    {
        nextThread->isAlive = false;
        nextThread = nextThread->nextTcb;
    }

    nextThread->nextTcb = nextThread;
    nextThread->prevTcb = nextThread;
    NumberOfThreads = 1;

    EndCriticalSection(criticalSection);
    return NO_ERROR;
}

/*********************************************** Public Functions *********************************************************************/
