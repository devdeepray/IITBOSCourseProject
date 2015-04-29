#include <geekos/string.h>
#include <geekos/screen.h>
#include <geekos/oscourse/fsysdef.h>

typedef struct DirHeader
{
	unsigned int parentInode; // will point to parent directory
	unsigned int numEntries; // number of entries in "t" block of the directory
} DirHeader;

struct DirEntry
{
	char fname[MAX_FILE_NAME_LENGTH];
	int inode_num;
} DirEntry;


// We assume we have two open functions
// One opens file in some path, other opens file pointed to by the Inode
// inodeNum = -1 if file does not exist


int existsFileName(Inode* pwd, const char *fileName, int *inodeNum ,int *entryNum);


typedef struct Path
{
	// Assumes the path passed in the constructor is well formed.
	// Examples: /
	//         : /home/
	//         : /home/vishal/desktop

	char fname[MAX_FILE_NAME_LENGTH];
	struct Path *childPath;// = NULL;
} Path;

int Create_Path(char *path, Path *path_obj);

int Free_Path(Path *path_obj);

int Get_Inode_From_Path(Path path, int* inode_Num);
                
int Make_Dir_With_Inode(Inode *parentInode, char* fileName, int pinodeNum);

int Make_Dir(char* parentPath, char* fileName);

int Remove_Dir_With_Inode(Inode *parentInode, char *fileName, int pinodeNum);//Assumption Only Empty Directories

int Remove_Dir( char* parentPath, char* fileName);
