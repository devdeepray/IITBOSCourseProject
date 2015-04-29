extern HashTable oft_hash;

typedef struct Global_Fcb {
	int inodeNo;	// Should be fixed in the cache
	int isWriting;
	Inode* inode;
	int numReading;
} Global_fcb;

int Init_Global_Fcb(Global_fcb *fcb, int inodeNo);

// The inode will always be in the cache
int Check_Perms(char op, Inode *buf);

// Creates a new Global_fcb if it does not exist. Otherwise updates existing fcb
int Open_Oft(int inodeNo, char op, Inode **inode);

int Close_Oft(int inodeNo, char op);

int Init_Oft();

/*************************************************************************************************/

enum SeekStart {F_START, F_CURR, F_END}

extern int fdCount;
extern HashTable lft_hash;

typedef Local_Fcb {
	int seekPos;
	char intendedOp;
	int inodeNo;
	Inode *inode;
} Local_Fcb;

int Init_Local_Fcb(LocalFcb *fcb, int inodeNo, char op, Inode *inode);

int Get_Next_Fd(int *nextFd);

int Init_Lft();

int Open_Lft(int inodeNo, char op, int *fd);

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

int Init_Inode_Metadata(InodeMetaData *md, char* path);

int Open(char* path, char op, int* fd);

int Close(int fd);

int Seek(int fd, int byteLoc, SeekStart pos);

//does the reading when the exact info is available

int readLow(Inode* inode, char* buf, int seekPos, int nbytes);

// buf should be preallocated
int Read(int fd, char* buf, int nbytes);

int writeLow(Inode* inode, const char* buf, int seekPos, int nbytes);

int Write(int fd, const char* buf, int nbytes);

int Close(int fd);

int Resize(int fd, int newSize);
