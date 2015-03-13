#include <geekos/errno.h>
#include <geekos/syscall.h>
#include <fileio.h>
#include <string.h>


/* Syscalls for sim_disk virtual disk */
DEF_SYSCALL(Vir_Seek, SYS_VIR_SEEK, int, (int cylinder, int track, int block),
				int arg1 = cylinder;
				int arg2 = track;
				int arg3 = block;
				, SYSCALL_REGS_3)

DEF_SYSCALL(Vir_Read, SYS_VIR_READ, int, (int bytes_to_read, char* buf),
				int arg1 = bytes_to_read;
				char* arg2 = buf;
				, SYSCALL_REGS_2)

DEF_SYSCALL(Vir_Write, SYS_VIR_WRITE, int, (int bytes_to_write, char* buf),
				int arg1 = bytes_to_write;
				char* arg2 = buf;
				, SYSCALL_REGS_2)