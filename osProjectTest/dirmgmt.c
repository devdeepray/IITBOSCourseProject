#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_NAME 20
#define BLOCK_SIZE 4096
#define MAX_PATH_LENGTH 200

struct DirHeader
{
	unsigned int parentInode; // will point to parent directory
	unsigned int numEntries; // number of entries in "t" block of the directory
};

struct DirEntry
{
	char fname[MAX_FILE_NAME_LENGTH];
	int inode_num;
};


// We assume we have two open functions
// One opens file in some path, other opens file pointed to by the Inode
// inodeNum = -1 if file does not exist


int existsFileName(Inode* pwd, const char *fileName, int *inodeNum ,int *entryNum)
{
	//int *f = Open(pwd);
	DirHeader dhead;
	//int numread;
	int rc;
	*inodeNum = -1;
	*entryNum = -1;
	
	//numread = readBytes(f, &dhead, sizeof(dhead));
	int read = 0;
	rc = readLow(pwd, &dhead, read, sizeof(dhead));
	read += sizeof(dhead);
	//if(numread < sizeof(dhead))
	if(rc != 0)
	{
		printf("existsFileName: Didn't work readLow\n");
		//closeFile(f);
		return -1; // error
	}
	//while(!feof(f))
	while((*entryNum) + 1 < dhead.numEnteries)
	{
		DirEntry tmp;
		//numread = readBytes(f, &tmp, sizeof(tmp));
		rc = readLow(pwd, &tmp, read, sizeof(tmp));
		read += sizeof(tmp);
		//if(numread < sizeof(tmp))
		if(rc != 0)
		{
			printf("existsFileName: Didn't work readLow after the 1st time\n");
			//closeFile(f);
			return -1; // error
		}
		*entryNum++;
		if(strcmp(tmp.fname, fileName) == 0)
		{
			*inodeNum = tmp.inode_num;
			break;
		}
	}
	//closeFile(f);
	return 0;
}


struct Path
{
	// Assumes the path passed in the constructor is well formed.
	// Examples: /
	//         : /home/
	//         : /home/vishal/desktop

	char fname[MAX_FILE_NAME_LENGTH];
	Path* childPath;
	Path()
	{
			childPath = NULL;
	}
	Path(char* path)
	{
		childPath = NULL;
		strcpy(fname, "root");
		char tmpPath[MAX_PATH_LENGTH];
		strcpy(tmpPath, path);
		Path* curNode = this;
		int index = 0;
		while(true)
		{
			if(tmpPath[index] == '\0' || (tmpPath[index] == '/' && tmpPath[index +1 ] == '\0'))
			{
				break;
			}
			else
			{
				curNode->childPath = new Path();
				curNode = curNode->childPath;
				int next_slash = index + 1;
				while(true)
				{
					if(tmpPath[next_slash] == '/' || tmpPath[next_slash] == '\0')
					{
						break;
					}                                
					next_slash++;
				}
				char tmpRep = tmpPath[next_slash];
				tmpPath[next_slash] = '\0';
				strcpy(curNode->fname, &tmpPath[index + 1]);
				tmpPath[next_slash] = tmpRep;        
				index = next_slash;
			}
		}		
	}
	// TODO: Shouldn't the destructor be written iteratively 
	~Path()
	{
		if(childPath != NULL)
		{
			delete childPath;
		}
	}
};


int getInodeFromPath(Path path, Inode** inode, int* inode_Num)
{
	if(path == NULL)
	{
		return -1;
	}
	int rc;
	Inode* curInode;
	*inode_Num = 0;
	Get_Inode_Into_Cache(*inode_Num, &curInode);
	if(Check_Perms('r', curInode) != 0) {
		printf("getInodeFromPath: Permission check failed \n");
		Unfix_Inode_From_Cache(*inode_Num);
		return -1;
	}

	//getRootInode(&curInode);
	Path* curPath = path.childPath;
	while(true)
	{
		if(curPath == NULL)
		{
			*(inode) = curInode;
			return 0;
		} 
		else
		{
			int newInodeNum;
			int entryNum; // Not Used Here
			rc = existsFileName(curInode, curPath->fname, &newInodeNum, &entryNum);
			Unfix_Inode_From_Cache(*inode_Num);
			if(rc != 0)//on error inode** is garbage
			{
				if(curPath->childPath == NULL) {
					//So it seems the file is not there. Send value to make a new one
					return -256;
				}
				printf("getInodeFromPath:existsFileName failed (pain in existsFileName) \n");
				return rc;
			}
			if(newInodeNum < 0)
			{
				printf("getInodeFromPath:existsFileName failed wrong newInodeNum \n");
				return -1; // TODO errors
			}
			//Unfix_Inode_From_Cache(*inode_Num);
			*inode_Num = newInodeNum;
			Get_Inode_Into_Cache(*inode_Num, &curInode);
			if(Check_Perms('r', curInode) != 0) {
				printf("getInodeFromPath: Permission check failed \n");
				Unfix_Inode_From_Cache(*inode_Num);
				return -1;
			}
			//getInode(inodeNum, &curInode);
			curPath = curPath->childPath;
		}
}
                
int makeDir(Inode *parentInode, char* fileName, int pinodeNum)
{
	// checking whether file already exists
	int inodeNum;
	int entryNum; // Not Used Here
	int rc = existsFileName(parentInode, fileName, &inodeNum, &entryNum);
	if( rc != 0) {
		printf("makeDir: Error in existsFileName \n");
		return rc;
	}
	if(inodeNum != -1) 
	{
		printf("makeDir: File Already exists code 122 \n");
		return 122;
		//Error file already exists
		//TODO: return an appropriate code
	}
	
	// Basically creating a new file
	int newInodeNum;
	InodeMetaData meta_data;
	strcpy(&meta_data.filename,fileName); // assuming filename is null terminated
	meta_data.group_id = CURRENT_THREAD->groupId;
	meta_data.user_id = CURRENT_THREAD->userId;
	meta_data.isDirectory = 1;
	meta_data.file_size = sizeof(DirHeader); // check if size of works
	metaData.permissions = 7; //Set them permissions
	metaData.permissions = metaData->permissions | (7 << 3);
	metaData.permissions = metaData->permissions | (3 << 6);
	Create_New_Inode(meta_data, &newInodeNum);
	if( rc != 0 ) {
		printf("Create_New_Inode failed in makeDir oh No!\n");
		return rc;
	}
	
	// Append entry in the parent node and update numEntries
	//File *f = openFileInode(parentInode,'w');
	
	
	// Update numEntries in the header
	DirHeader dhead;
	//int numread;
	//numread = readBytes(f, &dhead, sizeof(dhead));
	int read = 0;
	rc = readLow(parentInode, &dhead, read, sizeof(dhead));
	read += sizeof(dhead);
	//if( numread < sizeof(dhead) )
	if(rc != 0)
	{
		//Error
		printf("MakeDir: reading inode failed\n");
		return -1;
	}
	dhead.numEntries++;	
	//fseek(f,sizeof(dhead.parentInode),SEEK_SET);
	writeLow(parentInode, &dhead, 0, sizeof(dhead));

	//fWrite(f,(char*)(&dhead.numEntries),sizeof(dhead.numEntries));
	
	//Add new entry
	DirEntry newEntry;
	strcpy(&newEntry.fname,fileName); // assuming filename is null terminated
	newEntry.inode_num = newInodeNum;
	//fseek(f,0,SEEK_END); // seeking to the end of file
	//fWrite(f,(char*)newEntry,sizeof(DirEntry));
	//Write
	int oldSize = (parentInode->meta_data).file_size;
	Allocate_Upto(pinodeNum , (parentInode->meta_data).file_size + sizeof(newEntry));
	writeLow(parentInode, &newEntry, oldSize, sizeof(newEntry));
	//closeFile(f);
	return 0;
}

int makeDir(char* parentPath, char* fileName)
{
	Path pPath(parentPath);
	Inode parentInode;
	int inodeNum;
	int rc = getInodeFromPath(pPath,&parentInode, &inodeNum);
	if( rc != 0)
	{        
		printf("makeDir: error or path doesn't exist\n");
		return -1;
		// Error, or parent path does not exist
	}
	if(Check_Perms('w', &parentInode) != 0) {
				printf("makeDir: Permission check failed \n");
				Unfix_Inode_From_Cache(*inodeNum);
				return -1;
	}
	rc = makeDir(parentInode,fileName, inodeNum);
	rc = rc | Set_Dirty(inodeNum);
	rc = rc | Unfix_Inode_From_Cache(inodeNum);
	return rc;
} 

int removeDir(Inode parentInode, char *fileName, int pinodeNum)//Assumption Only Empty Directories
{
	int inodeNum;
	int entryNum;
    int rc = existsFileName(parentInode,fileName,&inodeNum,&entryNum);
    if( rc != 0)
    {
    	printf("removeDir:File Exists failed\n");
		// Error
		return rc;
	}
	if( inodeNum == -1)
	{
		printf("removeDir: File Doesn't exist Code 122\n");
		// Error
		return 122;
		//File does not exist. Error
	}		    
	
	// TODO: If the file is a directory. We need to check whether the directory is actually empty
	//if(inodeNum ) Check if the dir is empty
	Inode delInode;
	Get_Inode_Into_Cache(inodeNum, &delInode);
	if((!(delInode.meta_data).isDirectory) || (delInode.meta_data).file_size != sizeof(DirHeader)){
		rc = 32;
		return rc;
	}
	Free_Inode(inodeNum);
	// Updating numEntries
	//File *f = openFileInode(parentInode);
	//DirHeader dhead;
	//int numread;
	//numread = readBytes(f, &dhead, sizeof(dhead));
	//if(numread < sizeof(dhead))
	/*{
		closeFile(f);
		return -1;
	}	
	dhead.numEntries--;	
	fseek(f,sizeof(dhead.parentInode),SEEK_SET);
	fWrite(f,(char*)(&dhead.numEntries),sizeof(dhead.numEntries));
	*/
	// Removing The Entry
	DirHeader dhead;
	int read = 0;
	rc = readLow(parentInode, &dhead, read, sizeof(dhead));
	read += sizeof(dhead);
	if(rc != 0)
	{
		//Error
		printf("MakeDir: reading inode failed\n");
		return -1;
	}
	dhead.numEntries--;	
	
	DirEntry lastEntry;
	if(dhead.numEntries > 1)
	{
		//fseek(f,(dhead.numEntries-1)*sizeof(lastEntry),SEEK_CUR);
		//numread = readBytes(f, &lastEntry, sizeof(lastEntry));
		//numEnteries has already been reduced
		rc = readLow(parentInode, &lastEntry, (dhead.numEntries)*sizeof(lastEntry), sizeof(lastEntry));
		int offset = sizeof(dhead) + entryNum*sizeof(lastEntry);
		writeLow(parentInode, &lastEntry, offset, sizeof(lastEntry));
		//fseek(f,offset,SEEK_SET);
		//fWrite(f,(char*)(&lastEntry),sizeof(lastEntry));	
	}	
	
	// remove the last entry in the file
	Truncate_From(sizeof(lastEntry));
	
	//closeFile(f);
	return 0;
}

int removeDir( char* parentPath, char* fileName)
{
	Path pPath(parentPath);
	Inode parentInode;
	int inodeNum;
	int rc = getInodeFromPath(pPath, &parentInode, &inodeNum);
	if(rc != 0)
	{        
		printf("removeDir: Path Doesn't Exist! \n");
		return rc;
		// Error, or parent path does not exist
	}
	if(Check_Perms('w', &parentInode) != 0) {
				printf("removeDir: Permission check failed \n");
				Unfix_Inode_From_Cache(*inodeNum);
				return -1;
	}
	rc = removeDir(parentInode,fileName, inodeNum);
	rc = rc | Set_Dirty(inodeNum);
	rc = rc | Unfix_Inode_From_Cache(inodeNum);
	return rc;
}
