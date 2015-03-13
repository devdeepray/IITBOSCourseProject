/*
 * Keyboard driver
 * Copyright (c) 2001,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.15 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

/*
 * Information sources:
 * - Chapter 8 of _The Undocumented PC_, 2nd ed, by Frank van Gilluwe,
 *   ISBN 0-201-47950-8.
 * - Pages 400-409 of _The Programmers PC Sourcebook_, by Thom Hogan,
 *   ISBN 1-55615-118-7.
 */

/*
 * Credits:
 * - Peter Gnodde <peter@pcswebdesign.nl> added support for
 *   the CTRL and ALT modifiers
 */

/* nspring added partial e0 (101-key, right control) stuff */

/*
 * TODO list:
 * - Right now we're assuming an 83-key keyboard.
 *   Should add support for 101+ keyboards.
 * - Should toggle keyboard LEDs.
 */

#include <geekos/kthread.h>
#include <geekos/irq.h>
#include <geekos/timer.h>

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */

/*
 * Queue for diskIOrequests, in case they arrive faster than consumer
 * can deal with them.
 */
#define QUEUE_SIZE 256
#define QUEUE_MASK 0xff
#define NEXT(index) (((index) + 1) & QUEUE_MASK)

static bool ongoingIO = false;
static int waitingProcesses = 0;

/*
 * Wait queue for thread(s) waiting for disk events.
 */
static struct Thread_Queue disk_waitQueue;
static struct Thread_Queue diskread_waitQueue;


static __inline__ bool Is_Disk_Free(void) {
    return !ongoingIO;
}



static __inline__ void Start_DiskIO(void) {
	ongoingIO = true; 
}

static __inline__ void Stop_DiskIO(void) {
	ongoingIO = false;
}

int estimateTime(int bytes)
{
   return 4;
}

static void Wake_Reading_Thread(int ID)
{
//	Disable_Interrupts();
	Wake_Up_Head(&diskread_waitQueue);
//	Enable_Interrupts();
}

int Wait_For_Disk(bool mode, int bytes) {	
	//Print("Wait fr disk called");
	bool  iflag;

    
    iflag = Begin_Int_Atomic();
//Print("Starting loop");    
    do {
        
        if (Is_Disk_Free())
            break;
        else
	{
	    waitingProcesses++;
            Wait(&disk_waitQueue);
	    waitingProcesses--;
	}
    }
    while (true);
		
		//Print("Exit loop");
    int waitTime = estimateTime(bytes);
//		Disable_Interrupts();
    Start_Timer(waitTime, Wake_Reading_Thread);
//		Enable_Interrupts();
	//	Print("Binded callback");
    Start_DiskIO();
	//	Print("Waiting for read end");
    Wait(&diskread_waitQueue);
    Stop_DiskIO();
    if(waitingProcesses)
    {	
//      Disable_Interrupts();
      Wake_Up_Head(&disk_waitQueue);
//      Enable_Interrupts();
    }
    End_Int_Atomic(iflag);

    return 0;
}
