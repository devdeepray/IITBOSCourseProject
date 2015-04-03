/* This file is for initialization of the disk structure when the disk 
 * is formatted, and loading the superblock when the OS is booted up.
 *
 * Structures required:
 * 
 * 1. Struct for the superblock
 * -> Will contain the entire disk structure. An instance of this struct
 * 		is needed globally every time the system boots
 * -> The same struct can be used for configuring a format. The format 
 * 		function takes a superblock and formats disk acc. to given
 * 		superblock.
 * 
 * Functions required:
 *
 *  1. Format_File_System(SuperBlock superblock)
 * -> Uses virtualdisk.h to read/write raw blocks on the disk and format 
 * 		it to given superblock specs.
 * -> Particulars: Write superblock, block bitmap. Allocate for inode
 * 		bitmap and inodes, and call inode initialization
 * 2. Init_File_System()
 * -> Loads the superblock from the disk, and copies it to the global 
 * 		instance
 * -> Calls initialize on all other things that need initializing, eg. 
 * 		Global file table, inode manager, fscache.
 * 
 * Available headers:
 * 1. virtualdisk.h
 * !!! Should not use any other higher level functions !!!
 */
