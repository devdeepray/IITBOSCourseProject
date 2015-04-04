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



#include <geekos/ktypes.h>
#include <geekos/vfs.h>
#include <geekos/fileio.h>

#ifdef GEEKOS

#define MAX_FILE_SIZE 512
#define DISK_CONFIG_FILE "/c/diskconf.txt"
#define DISK_FILE "/c/mydisk.img"







struct {
	// current head pos
	int cylinder;
	bool is_reading;

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
int Wait_For_Disk(float time);

// Loads config, and initiates IOCS
int Init_Sim_Disk();

// Reads n_blocks blocks starting from block_num into buf
int Read_From_Disk(char* buf, int block_num, int n_blocks);

// Writes n_blocks blocks starting from block_num from buf
int Write_To_Disk(char* buf, int block_num, int n_blocks);

// Estimates time needed to seek and get to right block
float Estimate_Time(int block_number, int n_blocks);

#endif /* GEEKOS */

#endif /* GEEKOS_VIRTUALDISK_H */
