/*
 * Keyboard driver
 * Copyright (c) 2001, David H. Hovemeyer <daveho@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 *
 * $Revision: 1.13 $
 * 
 */

#ifndef GEEKOS_VIRTUALDISK_H
#define GEEKOS_VIRTUALDISK_H

#include <geekos/ktypes.h>


#ifdef GEEKOS

/*
 * Public functions
 */
int Wait_For_Disk(bool, int);

#endif /* GEEKOS */

#endif /* GEEKOS_KEYBOARD_H */
