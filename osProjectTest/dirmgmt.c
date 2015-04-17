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
int existsFileName(Inode pwd, const char *fileName, int *inodeNum ,int *entryNum;)
{
	File *f = openFileInode(pwd);
	DirHeader dhead;
	int numread;
	*inodeNum = -1;
	*entryNum = -1;
	numread = readBytes(f, &dhead, sizeof(dhead));
	if(numread < sizeof(dhead))
	{
		closeFile(f);
		return -1; // error
	}
	while(!feof(f))
	{
		DirEntry tmp;
		numread = readBytes(f, &tmp, sizeof(tmp));
		if(numread < sizeof(tmp))
		{
			closeFile(f);
			return -1; // error
		}
		*entryNum++;
		if(strcmp(tmp.fname, fileName) == 0)
		{
			*inodeNum = tmp.inode_num;
			break;
		}
	}
	closeFile(f);
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


int getInodeFromPath(Path path, Inode* inode)
{
	if(path == NULL)
	{
		return -1;
	}

	Inode curInode;
	getRootInode(&curInode);
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
			int inodeNum;
			int entryNum; // Not Used Here
			int rc = existsFileName(curInode, curPath->fname, &inodeNum, &entryNum);
			if(rc != 0)
			{
				return rc;
			}
			if(inodeNum < 0)
			{
				return -1; // TODO errors
			}
			getInode(inodeNum, &curInode);
			curPath = curPath->childPath;
		}
}
                
int makeDir( Inode parentInode, char* fileName)
{
	// checking whether file already exists
	int inodeNum;
	int entryNum; // Not Used Here
	int rc = existsFileName(parentInode, fileName, &inodeNum, &entryNum);
	if( rc != 0) return rc;
	if( inodeNum != -1) 
	{
		//Error file already exists
		//TODO: return an appropriate code
	}
	
	// Basically creating a new file
	int newInodeNum;
	rc = AllocInode(&newInodeNum); // The assumption is that the File Header will be made by this function
	if( rc != 0 ) return rc;

	// Append entry in the parent node and update numEntries
	File *f = openFileInode(parentInode,'w');
	
	// Update numEntries in the header
	DirHeader dhead;
	int numread;
	numread = readBytes(f, &dhead, sizeof(dhead));
	if( numread < sizeof(dhead) ) 
	{
		//Error
		closeFile(f);
		return -1;
	}
	dhead.numEntries++;	
	fseek(f,sizeof(dhead.parentInode),SEEK_SET);
	fWrite(f,(char*)(&dhead.numEntries),sizeof(dhead.numEntries));
	
	//Add new entry
	DirEntry newEntry;
	strcpy(&newEntry.fname,fileName); // assuming filename is null terminated
	newEntry.inode_num = newInodeNum;
	fseek(f,0,SEEK_END); // seeking to the end of file
	fWrite(f,(char*)newEntry,sizeof(DirEntry));
	
	closeFile(f);
	return 0;
}

int makeDir(char* parentPath, char* fileName)
{
	Path pPath(parentPath);
	Inode parentInode;
	int rc = getInodeFromPath(pPath,&parentInode);
	if( rc != 0)
	{        
		// Error, or parent path does not exist
	}
	rc = makeDir(parentInode,fileName);
	return rc;
} 


int removeDir(Inode parentInode, char *fileName)
{
	int inodeNum;
	int entryNum;
    int rc = existsFileName(parentInode,fileName,&inodeNum,&entryNum);
    if( rc != 0)
    {
		// Error
		return rc;
	}
	if( inodeNum == -1)
	{
		//File does not exist. Error
	}		    
	
	// TODO: If the file is a directory. We need to check whether the directory is actually empty
	
	DeAlloc(inodeNum);
	
	// Updating numEntries
	File *f = openFileInode(parentInode);
	DirHeader dhead;
	int numread;
	numread = readBytes(f, &dhead, sizeof(dhead));
	if(numread < sizeof(dhead))
	{
		closeFile(f);
		return -1;
	}	
	dhead.numEntries--;	
	fseek(f,sizeof(dhead.parentInode),SEEK_SET);
	fWrite(f,(char*)(&dhead.numEntries),sizeof(dhead.numEntries));
	
	// Removing The Entry
	DirEntry lastEntry;
	if(dhead.numEntries > 1)
	{
		fseek(f,(dhead.numEntries-1)*sizeof(lastEntry),SEEK_CUR);
		numread = readBytes(f, &lastEntry, sizeof(lastEntry));
		int offset = sizeof(dhead) + entryNum*sizeof(lastEntry);
		fseek(f,offset,SEEK_SET);
		fWrite(f,(char*)(&lastEntry),sizeof(lastEntry));	
	}	
	
	// remove the last entry in the file
	ftruncate(sizeof(lastEntry));
	
	closeFile(f);
	return 0;
}

int removeDir( char* parentPath, char* fileName)
{
	Path pPath(parentPath);
	Inode parentInode;
	int rc = getInodeFromPath(pPath,&parentInode);
	if( rc != 0)
	{        
		// Error, or parent path does not exist
	}
	rc = removeDir(parentInode,fileName);
	return rc;
}
