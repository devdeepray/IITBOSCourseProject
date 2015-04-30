#include <geekos/oscourse/oft.h>
#include <geekos/errno.h>
#include <geekos/syscall.h>

DEF_SYSCALL(Vir_File_Open, SYS_VIR_FILE_OPEN, int, (char* path, char* fname, int op, int* fd),
				char *arg0 = path;
				char *arg1 = fname;
				int arg2 = op;
				int *arg3 = fd;
				, SYSCALL_REGS_4)

DEF_SYSCALL(Vir_File_Read, SYS_VIR_FILE_READ, int, (int fd, char* buf, int nbytes),
				int arg0 = fd;
				char *arg1 = buf;
				int arg2 = nbytes;
				, SYSCALL_REGS_3)

DEF_SYSCALL(Vir_File_Write, SYS_VIR_FILE_WRITE, int, (int fd, const char* buf, int nbytes),
				int arg0 = fd;
				const char *arg1 = buf;
				int arg2 = nbytes;
				, SYSCALL_REGS_3)

DEF_SYSCALL(Vir_File_Seek, SYS_VIR_FILE_SEEK, int, (int fd, int byteLoc, int pos),
				int arg0 = fd;
				int arg1 = byteLoc;
				int arg2 = pos;
				, SYSCALL_REGS_3)

DEF_SYSCALL(Vir_File_Resize, SYS_VIR_FILE_RESIZE, int, (int fd, int newSize),
				int arg0 = fd;
				int arg1 = newSize;
				, SYSCALL_REGS_2)


DEF_SYSCALL(Vir_File_Close, SYS_VIR_FILE_CLOSE, int, (int fd),
				int arg0 = fd;
				, SYSCALL_REGS_1)

DEF_SYSCALL(Vir_Disk_Accesses, SYS_VIR_DISK_ACCESSES, void,(void),
				, SYSCALL_REGS_0)


DEF_SYSCALL(Vir_File_Mkdir, SYS_VIR_FILE_MKDIR, int, (char* parentPath, char* fileName),
				char* arg0 = parentPath;
				char* arg1 = fileName;
				, SYSCALL_REGS_2)
				
				
DEF_SYSCALL(Vir_File_Del, SYS_VIR_FILE_DEL, int, (char* parentPath, char* fileName),
				char* arg0 = parentPath;
				char* arg1 = fileName;
				, SYSCALL_REGS_2)
				
				
DEF_SYSCALL(Vir_Get_Into_Cache, SYS_GET_INTO_CACHE, int, (int blocknum),
				int arg0 = blocknum;
				, SYSCALL_REGS_1)

				
DEF_SYSCALL(Vir_Unfix_From_Cache, SYS_UNFIX_FROM_CACHE, int, (int blocknum),
				int arg0 = blocknum;
				, SYSCALL_REGS_1)


