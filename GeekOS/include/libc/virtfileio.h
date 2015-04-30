#ifndef VIRTFILEIO_H
#define VIRTFILEIO_H

/* Syscalls for virtual fs */
int Vir_File_Open(char* path,char* fname, char op, int* fd);
int Vir_File_Read(int fd, char* buf, int nbytes);
int Vir_File_Write(int fd, const char* buf, int nbytes);
int Vir_File_Seek(int fd, int byteLoc, int pos);
int Vir_File_Resize(int fd, int newSize);
int Vir_File_Close(int fd);
int Vir_File_Mkdir(char* parentPath, char* fileName);
int Vir_File_Del(char* parentPath, char* fileName);
int Vir_Get_Into_Cache(int blocknum);
int Vir_Unfix_From_Cache(int blocknum);
void Vir_Disk_Accesses();

#endif /* VIRTFILEIO_H */
