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

#define MAXFILESIZE 512
#define DISK_CONFIG_FILE "/c/diskconf.txt"

/*
 * Public functions
 */
int Wait_For_Disk(bool, int);


struct {
	int cylinder;
	
	/* Disk characteristics */
	int block_size;
	int tot_blocks;
	int tot_tracks_per_cyl;
	int track_cap;
	int time_to_shift_cyl;
	int rot_time;
	int block_read_time;
} disk_state;


char file[MAXFILESIZE];
int fileSize;
extern int pos_in_config_file;

void Init_Sim_Disk();
int estimateTime(int no_of_bytes);

#endif /* GEEKOS */

#endif /* GEEKOS_KEYBOARD_H */
