/*
 * Simulated disk driver for File System project
 * Copyright (c) 2015, Group 10 CSE 2016
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kthread.h>
#include <geekos/virtualdisk.h>
#include <geekos/irq.h>
#include <geekos/timer.h>
#include <geekos/string.h>
#include <geekos/fileio.h>
#include <geekos/vfs.h>

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */




// Used for reading the config file
char file[MAX_CONFIG_FILE_SIZE];
int fileSize;
int pos_in_config_file;
struct File *disk_file;


/*
 * Queue for diskIOrequests, in case they arrive faster than consumer
 * can deal with them.
 */

// Number of processes waitin for the disk
static int waitingProcesses = 0;

// Wait for access to disk
static struct Thread_Queue disk_waitQueue;

// Wait for read to get complete (IO_END)
static struct Thread_Queue diskread_waitQueue;



// Reads a line from the array 
// Helper
int readLineFileArray(char* buf) {
	int i = 0;

	while(1) {
		if (pos_in_config_file >= fileSize) {
			pos_in_config_file = 0;
			buf[i] = 0;
			return -1;
		}
		if (file[pos_in_config_file] == '\n') {
			pos_in_config_file++;
			buf[i] = 0;
			return 0;
		}
		else {
			buf[i] = file[pos_in_config_file];
			i += 1;
			pos_in_config_file += 1;
		}
	}
}


/*
file structure on different lines
	int bytes_per_block
	int blocks_per_track
	int tracks_per_cylinder
	int total_cylinders;
	float avg_rot_time;
	float avg_seek_time;
	float block_read_time;
*/


// It opens the disk file, and loads the disk config
int Init_Sim_Disk() {
	struct File *file_struct;
 	int rc = Open(DISK_CONFIG_FILE, O_READ, &file_struct);
	if(rc) return rc;
	fileSize = Read(file_struct, file, MAX_CONFIG_FILE_SIZE);
	Close(file_struct);
	Print("In Init sim disk");
	int a = 0;
	char buf[20];
	readLineFileArray(buf);
	disk_hw_data.bytes_per_block = atoi(buf); // Bytes per block

	readLineFileArray(buf);
	disk_hw_data.blocks_per_track = atoi(buf); // Blocks per track

	readLineFileArray(buf);
	disk_hw_data.tracks_per_cylinder = atoi(buf); // Tracks per cyl

	readLineFileArray(buf);
	disk_hw_data.tot_cylinders = atoi(buf); // Total cyl

	readLineFileArray(buf);
	disk_hw_data.avg_rot_time = atoi(buf); // In millis

	readLineFileArray(buf);
	disk_hw_data.avg_seek_time = atoi(buf); // In millis

	readLineFileArray(buf);
	disk_hw_data.block_read_time = atoi(buf); // In millis per 100 blocks

	disk_hw_data.cylinder = 0;
	
	rc = Open(DISK_FILE, O_READ | O_WRITE, &disk_file);
	return rc;
}


int Read_From_Disk(char *buf, int block_num, int n_blocks)
{
	int rc = Seek(disk_file, block_num * disk_hw_data.bytes_per_block);
	if(rc) return rc;
	rc = Read(disk_file, buf, n_blocks * disk_hw_data.bytes_per_block);
	return rc;
}

int Write_To_Disk(char* buf, int block_num, int n_blocks)
{
	int rc = Seek(disk_file, block_num * disk_hw_data.bytes_per_block);
	if(rc) return rc;
	rc = Write(disk_file, buf, n_blocks * disk_hw_data.bytes_per_block);
	return rc;
}

// Calculates estimate of disk seek time for block num and number of blocks
float Estimate_Time(int block_number, int n_blocks)
{		
	int new_cyl_num = block_number / (disk_hw_data.blocks_per_track * disk_hw_data.tracks_per_cylinder);
	int delta_cyl = new_cyl_num - disk_hw_data.cylinder;
	delta_cyl = delta_cyl>0?delta_cyl:-delta_cyl;
	float seek_time = 2.0f * disk_hw_data.avg_seek_time * ((float)delta_cyl / disk_hw_data.tot_cylinders);
	float res = seek_time + disk_hw_data.avg_rot_time + n_blocks * disk_hw_data.block_read_time / 100.0f;
	Print("%f", res);
	return res;
}


static void Wake_Reading_Thread(int ID)
{
	Wake_Up_Head(&diskread_waitQueue);
	Cancel_Timer(ID);
}


// Get the disk lock, and then sleep for certain amount of time
int Wait_For_Disk(float time) {	
	bool  iflag;
	iflag = Begin_Int_Atomic();

    do {
			//Print("Starting loop %d\n", CURRENT_THREAD->pid);    
        
		if (!(disk_hw_data.is_reading))
		{
			//Print("Disk is free %d\n", CURRENT_THREAD->pid);
			break;
		}
		else
		{
			waitingProcesses++;
			Wait(&disk_waitQueue);
			waitingProcesses--;
		}
    } while (true);

	disk_hw_data.is_reading = true;
	//Start_DiskIO();
   	//Print("wait time %d %d\n", CURRENT_THREAD->pid, waitTime);
	Start_Timer(time, Wake_Reading_Thread);
		
	//Print("Waiting for read end");
    Wait(&diskread_waitQueue);
  	//Print("Read over");
	disk_hw_data.is_reading = false;
    
    if(waitingProcesses)
    {	
		Wake_Up_Head(&disk_waitQueue);
    }
    End_Int_Atomic(iflag);

    return 0;
}
