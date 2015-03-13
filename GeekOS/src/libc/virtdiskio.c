#include <geekos/errno.h>
#include <geekos/syscall.h>
#include <fileio.h>
#include <string.h>


/* Syscalls for sim_disk virtual disk */
DEF_SYSCALL(Vir_Seek, SYS_VIR_SEEK, int, (int cylinder, int track, int block),
				int arg0 = cylinder;
				int arg1 = track;
				int arg2 = block;
				, SYSCALL_REGS_3)

DEF_SYSCALL(Vir_Read, SYS_VIR_READ, int, (int bytes_to_read, char* buf),
				int arg0 = bytes_to_read;
				char* arg1 = buf;
				, SYSCALL_REGS_2)

DEF_SYSCALL(Vir_Write, SYS_VIR_WRITE, int, (int bytes_to_write, char* buf),
				int arg0 = bytes_to_write;
				char* arg1 = buf;
				, SYSCALL_REGS_2)
