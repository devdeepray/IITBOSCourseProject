#include <virtfileio.h>
#include <conio.h>

int main() {
	int fd1;
	char dir3[] = "/";
	char fname3[] = "vishal.txt";
	Vir_Disk_Accesses();
	
	Vir_File_Open(dir3, fname3, 2, &fd1);
	Vir_File_Close(fd1);
		Vir_Disk_Accesses();
	
	Vir_File_Open(dir3, fname3, 2, &fd1);
	Vir_File_Close(fd1);
		Vir_Disk_Accesses();
	
	Vir_File_Open(dir3, fname3, 2, &fd1);
	Vir_File_Close(fd1);
	Vir_Disk_Accesses();
	
	return 0;
}
	
