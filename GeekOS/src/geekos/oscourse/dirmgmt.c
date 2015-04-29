#include <geekos/oscourse/dirmgmt.h>

// We assume we have two open functions
// One opens file in some path, other opens file pointed to by the Inode
// inodeNum = -1 if file does not exist

int Create_Path(char *path, Path *path_obj) {
	path_obj->childPath = NULL;
	strcpy(path_obj->fname, "root");
	char tmpPath[MAX_PATH_LENGTH];
	strcpy(tmpPath, path);
	Path* curNode = path_obj;
	int index = 0;
	while(true)
	{
		if(tmpPath[index] == '\0' || (tmpPath[index] == '/' && tmpPath[index +1 ] == '\0'))
		{
			break;
		}
		else
		{
			curNode->childPath = (Path*)Malloc(sizeof(Path));
			curNode = curNode->childPath;
			curNode->childPath = NULL;
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

int Free_Path(Path *path_obj) {
	if(path_obj->childPath != NULL)
	{
		Free_Path(path_obj->childPath);
		Free(path_obj);
	}
	return 0;
}


// If file does not exist, inode num returned is -1, error code is 0
int Exists_File_Name(Inode* pwd, const char *fileName, int *inodeNum ,int *entryNum)
{
	//int *f = Open(pwd);
	DirHeader dhead;
	//int numread;
	int rc;
	*inodeNum = -1;
	int curEntryNum = -1;
	*entryNum = -1;
	
	//numread = readBytes(f, &dhead, sizeof(dhead));
	int read = 0;
	rc = My_ReadLow(pwd, (char*)&dhead, read, sizeof(dhead));
	read += sizeof(dhead);
	if(rc != 0)
	{
		Print("Exists_File_Name: Didn't work readLow\n");
		return -1; 
	}
	while(curEntryNum + 1 < dhead.numEntries)
	{
		DirEntry tmp;
		rc = readLow(pwd, &tmp, read, sizeof(tmp));
		read += sizeof(tmp);
		if(rc != 0)
		{
			Print("Exists_File_Name: Didn't work readLow after the 1st time\n");
			return -1;
		}
		curEntryNum++;
		if(strcmp(tmp.fname, fileName) == 0)
		{
			*inodeNum = tmp.inode_num;
			*entryNum = curEntryNum;
			return 0;
		}
	}
	Print("File not found\n");
	return -1;
}


// If path does not exist: -1
int Get_Inode_From_Path(Path path, int* inode_num)
{
	int rc;
	Inode* curInode;
	int curInodeNum;
	Get_Inode_Into_Cache(0, &curInode);
	if(Check_Perms('r', curInode) != 0) {
		Print("Get_Inode_From_Path: Permission check failed \n");
		Unfix_Inode_From_Cache(0);
		return -1;
	}

	Path* curPath = path.childPath;
	while(true)
	{
		
		
		if(curPath == NULL)
		{
			Unfix_Inode_From_Cache(curInodeNum);
			*(inode_num) = curInodeNum;
			return 0;
		} 
		else
		{
			if(!(curInode->meta_data.is_directory))
			{
				Unfix_Inode_From_Cache(curInodeNum);
				Print("Arbit file in path name\n");
				return -1;
			}
			int newInodeNum;
			int entryNum; // Not Used Here
			rc = Exists_File_Name(curInode, curPath->fname, &newInodeNum, &entryNum);
			Unfix_Inode_From_Cache(curInodeNum);
			if(rc != 0)//on error inode** is garbage
			{
				Print("Get_Inode_From_Path:Exists_File_Name failed (pain in Exists_File_Name) \n");
				return -1;
			}
			
			curInodeNum = newInodeNum;
			Get_Inode_Into_Cache(*curInodeNum, &curInode);
			if(Check_Perms('r', curInode) != 0) {
				Print("Get_Inode_From_Path: Permission check failed \n");
				Unfix_Inode_From_Cache(*inode_Num);
				return -1;
			}
			curPath = curPath->childPath;
		}
	}
}
                
int Make_Dir_With_Inode(Inode *parentInode, char* fileName, int pinodeNum)
{
	// checking whether file already exists
	int inodeNum;
	int entryNum; // Not Used Here
	int rc = Exists_File_Name(parentInode, fileName, &inodeNum, &entryNum);
	if( rc != 0) {
		Print("Make_Dir: Error in Exists_File_Name \n");
		return rc;
	}
	
	// Basically creating a new file
	int newInodeNum;
	InodeMetaData meta_data;
	strcpy(&meta_data.filename,fileName); // assuming filename is null terminated
	meta_data.group_id = CURRENT_THREAD->group_id;
	meta_data.user_id = CURRENT_THREAD->user_id;
	meta_data.is_directory = 0x1;
	meta_data.file_size = sizeof(DirHeader); // check if size of works
	metaData.permissions = 0x7; //Set them permissions
	metaData.permissions = metaData->permissions | (7 << 3);
	metaData.permissions = metaData->permissions | (3 << 6);
	rc = Create_New_Inode(meta_data, &newInodeNum);
	if( rc != 0 ) {
		Print("Create_New_Inode failed in Make_Dir oh No!\n");
		return rc;
	}
	
	
	// Update numEntries in the header
	DirHeader dhead;
	
	rc = readLow(parentInode, &dhead, 0, sizeof(dhead));
	
	
	if(rc != 0)
	{
		//Error
		Print("Make_Dir: reading directory header failed\n");
		return -1;
	}
	dhead.numEntries++;	
	writeLow(parentInode, &dhead, 0, sizeof(dhead));
	
	
	//Add new entry
	DirEntry newEntry;
	strcpy(&newEntry.fname,fileName); // assuming filename is null terminated
	newEntry.inode_num = newInodeNum;
	int oldSize = (parentInode->meta_data).file_size;
	rc = Allocate_Upto(pinodeNum , oldSize + sizeof(newEntry));
	if(rc) 
	{
		Print("Failure allocating space for dir entry");
		
		return rc;
	}
		
	rc = writeLow(parentInode, &newEntry, oldSize, sizeof(newEntry));
	return rc;
}

int Make_Dir(char* parentPath, char* fileName)
{
	Path pPath;
	Create_Path(parentPath, &pPath);
	int inodeNum;
	int rc = Get_Inode_From_Path(pPath, &inodeNum);
	if( rc != 0 || inodeNum == -1)
	{        
		Print("Make_Dir: error or path doesn't exist\n");
		return -1;
		// Error, or parent path does not exist
	}
	Inode* parentInode ;
	rc = Get_Inode_Into_Cache(inodeNum, &parentInode);
	if(rc)
	{
		Print("dirmgmt.c/Make_Dir: Couldnt get directory inode into cache\n");
		return -1;
	}
	if(Check_Perms('w', parentInode) != 0) {
		Print("Make_Dir: Permission check failed \n");
		Unfix_Inode_From_Cache(inodeNum);
		return -1;
	}
	rc = Make_DirWithInode(parentInode,fileName, inodeNum);
	if(rc)
	{
		Print("dirmgmt/Make_Dir: Couldnot make dir");
	}
	rc = rc | Unfix_Inode_From_Cache(inodeNum);
	return rc;
} 

int Remove_Dir_With_Inode(Inode *parentInode, char *fileName, int pinodeNum)//Assumption Only Empty Directories
{
	int inodeNum;
	int entryNum;
    int rc = Exists_File_Name(parentInode,fileName,&inodeNum,&entryNum);
    if( rc != 0)
    {
    	Print("Remove_Dir:File Exists failed\n");
		return -1;
	}
	if( inodeNum == -1)
	{
		Print("Remove_Dir: File Doesn't exist Code 122\n");
		return -1;
	}		    
	
	// TODO: If the file is a directory. We need to check whether the directory is actually empty
	//if(inodeNum ) Check if the dir is empty
	Inode *delInode;
	rc = Get_Inode_Into_Cache(inodeNum, &delInode);
	if(rc || (!(delInode->meta_data).is_directory) || (delInode->meta_data).file_size != sizeof(DirHeader)){
		Print("Remove_Dir: Not directory, or not empty or get into cache error\n");
		return -1;
	}
	rc = Unfix_Inode_From_Cache(inodeNum);
	rc = rc | Free_Inode(inodeNum);
	if(rc)
	{
		Print("Remove_Dir: Could not free inode");
		return rc;
	}
	
	// Removing The Entry
	DirHeader dhead;
	rc = My_ReadLow(parentInode, &dhead, 0, sizeof(dhead));
	
	if(rc != 0)
	{
		//Error
		Print("Make_Dir: reading inode failed\n");
		return -1;
	}
	dhead.numEntries--;
	rc = My_WriteLow(parentInode, &dhead, 0, sizeof(dhead));
	
	DirEntry lastEntry;
	if(dhead.numEntries > 1)
	{
		rc = readLow(parentInode, &lastEntry, (dhead.numEntries)*sizeof(DirEntry), sizeof(DirEntry));
		int offset = sizeof(DirHeader) + entryNum*sizeof(DirEntry);
		writeLow(parentInode, &lastEntry, offset, sizeof(DirEntry));
	}
	
	// remove the last entry in the file
	rc = Truncate_From(sizeof(DirHeader) + dhead.numEntries*sizeof(DirEntry));
	if(rc)
	{
		Print("remodeDir: Error while trucating directory\n");
		return rc;
	}
	
	return 0;
}

int Remove_Dir( char* parentPath, char* fileName)
{
	Path pPath;
	Create_Path(parentPath, &pPath);
	Inode *parentInode;
	int inodeNum;
	int rc = Get_Inode_From_Path(pPath, &inodeNum);
	if(rc != 0 || inodeNum == -1)
	{        
		Print("Remove_Dir: Path Doesn't Exist! \n");
		return rc;
		// Error, or parent path does not exist
	}
	rc = Get_Inode_Into_Cache(inodeNum, &parentInode);
	if(rc)
	{
		Print("Remove_Dir: could not get inode into cache\n");
		return rc;
	}
	if(Check_Perms('w', parentInode) != 0) {
		Print("Remove_Dir: Permission check failed \n");
		Unfix_Inode_From_Cache(inodeNum);
		return -1;
	}
	rc = Remove_Dir(parentInode,fileName, inodeNum);
	rc = rc | Unfix_Inode_From_Cache(inodeNum);
	if(rc)
	{
		Print("Remove_Dir: Could not remove directory\n");
		return rc;
	}
	return 0;
}
