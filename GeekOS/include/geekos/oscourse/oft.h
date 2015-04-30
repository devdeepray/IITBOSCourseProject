#include <geekos/oscourse/hash.h>
#include <geekos/oscourse/inode.h>
#include <geekos/oscourse/dirmgmt.h>
#include <geekos/malloc.h>

extern Hashtable oft_hash;

typedef struct Global_Fcb {
	int inodeNo;	// Should be fixed in the cache
	int isWriting;
	int numReading;
} Global_Fcb;

int Init_Global_Fcb(Global_Fcb *fcb, int inodeNo);

// The inode will always be in the cache
int Check_Perms(int op, Inode *buf);

// Creates a new Global_Fcb if it does not exist. Otherwise updates existing fcb
int Open_Oft(int inodeNo, int op);

int Close_Oft(int inodeNo, int op);

int Init_Oft();

/*************************************************************************************************/

typedef enum SeekStart {F_START, F_CURR, F_END} SeekStart;

extern int fdCount;
extern Hashtable lft_hash;

typedef struct Local_Fcb {
	int seekPos;
	int intendedOp;
	int inodeNo;
	Inode *inode;
} Local_Fcb;

int Init_Local_Fcb(Local_Fcb *fcb, int inodeNo, int op, Inode *inode);

int Get_Next_Fd(int *nextFd);

int Init_Lft();

int Open_Lft(int inodeNo, int op, int *fd);

int Seek_Lft(int fileDes, int byteLoc, SeekStart pos);

int Resize_Lft(int fd, int newSize);

int Check_Read(int fileDes, int nbytes);

int Check_Write(int fileDes, int nbytes);

int Check_Resize(int fileDes);

int Close_Lft(int fileDes);

int Get_Inode_Lft(int fd, Inode **inode);

int Get_Seek_Pos_Lft(int fd, int *seekPos);

int Get_File_Size_Lft(int fd, int *fileSize);

/************************************************************************************************/

int Init_Inode_Metadata(InodeMetaData *md, char* fname);

int My_Open(char* path,char* fname, int op, int* fd);

int My_Close(int fd);

int My_Seek(int fd, int byteLoc, SeekStart pos);

//does the reading when the exact info is available

int My_ReadLow(Inode* inode, char* buf, int seekPos, int nbytes);

// buf should be preallocated
int My_Read(int fd, char* buf, int nbytes);

int My_WriteLow(Inode* inode, const char* buf, int seekPos, int nbytes);

int My_Write(int fd, const char* buf, int nbytes);

int My_Close(int fd);

int My_Resize(int fd, int newSize);
