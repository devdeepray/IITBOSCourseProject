#ifndef VIRTDISKIO_H
#define VIRTDISKIO_H

/* Syscalls for virtual disk sim_disk */
int Vir_Seek(int cylinder, int track, int block);
int Vir_Read(int bytes_to_read, char* buf);
int Vir_Write(int bytes_to_write, char* buf);


#endif /* VIRTDISKIO_H */
