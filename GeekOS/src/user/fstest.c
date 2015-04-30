#include <virtfileio.h>
#include <conio.h>

int main() {
	int fd1, fd2, fd3;
	char dir1[] = "/";
	char fname1[] = "vishal";
	
	
	
	//Vir_File_Open(dir1, fname1, 2, &fd1);
	//Vir_File_Close(fd1);
	Vir_File_Mkdir(dir1,fname1);
	char dir3[] = "/vishal/";
	char fname3[] = "mf3.txt";
	Vir_File_Open(dir3, fname3, 2, &fd1);
	//~ 
	Vir_File_Resize(fd1, 100);
	char dat1[] = "hello dmd1";
	Vir_File_Write(fd1, dat1, 10);
	Vir_File_Close(fd1);
	
	Vir_File_Open(dir3, fname3, 2, &fd1);
	Vir_File_Close(fd1);
	
	//~ 
	Vir_File_Open(dir3, fname3, 1, &fd1);
	char read[20];
	Vir_File_Read(fd1, read, 3);
	Print("1 %s\n", read);
	Vir_File_Close(fd1);
	
	Vir_File_Del(dir3, fname3);
	Vir_File_Del(dir3, fname3);
	Vir_File_Open(dir3, fname3, 1, &fd1);
	Vir_File_Read(fd1, read, 3);
	Print("1 %s\n", read);
	Vir_File_Close(fd1);
	Vir_File_Del(dir1, fname1);
	Vir_File_Del(dir1, fname1);
	Vir_Disk_Accesses();
	
	/*Vir_File_Open(dir1, fname1, 2, &fd1);
	Vir_File_Resize(fd1, 100);
	char dat1[] = "hello dmd1";
	Vir_File_Write(fd1, dat1, 10);
	char read[20];
	Vir_File_Seek(fd1, 7, 0);
	Vir_File_Read(fd1, read, 3);
	Print("1 %s\n", read);
	Vir_File_Close(fd1);
	
	Vir_File_Open(dir1, fname1, 2, &fd1);
	Vir_File_Write(fd1, dat1, 10);
	Vir_File_Seek(fd1, 7, 0);
	Vir_File_Read(fd1, read, 3);
	Print("1 %s\n", read);
	Vir_File_Close(fd1);*/
	/*char dir2[] = "/";
	char fname2[] = "mf2.txt";
	Vir_File_Open(dir2, fname2, 2, &fd2);
	Vir_File_Close(fd2);*/
	
	/*Vir_File_Resize(fd2, 100);
	char dat2[] = "hello dmd2";
	Vir_File_Write(fd2, dat2, 10);
	Vir_File_Seek(fd2, 6, 0);
	Vir_File_Read(fd2, read, 4);
	Print("2 %s\n", read);
	Vir_File_Close(fd2);*/
	
	
	/*Vir_File_Resize(fd3, 100);
	char dat3[] = "hello dmd3";
	Vir_File_Write(fd3, dat3, 10);
	Vir_File_Seek(fd3, 5, 0);
	Vir_File_Read(fd3, read, 5);
	Print("3 %s\n", read);
	Vir_File_Close(fd3);*/
	
	
	
	return 0;
}
