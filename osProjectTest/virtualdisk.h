/*
 * Simulated disk for File System project
 * Copyright (c) 2015, Group 10 CSE 2016
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 *
 * 
 */

#ifndef GEEKOS_VIRTUALDISK_H
#define GEEKOS_VIRTUALDISK_H

//~ #include <geekos/ktypes.h>
//~ #include <geekos/vfs.h>
//~ #include <geekos/fileio.h>

#include "fsysdef.h"
#include "geekoscover.h"


struct {
	// current head pos
	int cylinder;
	int is_reading;

	// Disk characteristics 
	int bytes_per_block;
	int blocks_per_track;
	int tracks_per_cylinder;
	int tot_cylinders;
	
	// All in milliseconds
	float avg_rot_time; 
	float avg_seek_time;
	float block_read_time;
} disk_hw_data;


/*
 * Public functions
 */
 
 
// Waits for disk to become available, and then sleeps for time
//~ int Wait_For_Disk(float time);

// Loads config, and initiates IOCS
// Returns: -1 if Initialization error. Prints error. 
int Init_Sim_Disk();

// Reads n_blocks blocks starting from block_num into buf
// Returns: -1 if Initialization error. Prints error. 
int Read_From_Disk(char* buf, int block_num, int n_blocks);

// Writes n_blocks blocks starting from block_num from buf
// Returns: -1 if Initialization error. Prints error. 
int Write_To_Disk(char* buf, int block_num, int n_blocks);

// Shuts down the disk system
// Returns: -1 if Initialization error. Prints error. 
int Shut_Down_Sim_Disk();

// Estimates time needed to seek and get to right block
//~ float Estimate_Time(int block_number, int n_blocks);


#endif /* GEEKOS_VIRTUALDISK_H */
