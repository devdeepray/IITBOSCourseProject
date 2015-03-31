
#define MAX_FILE_NAME 20
#define BLOCK_SIZE 4096

struct DirEntry
{
	char fileName[MAX_FILE_NAME];
	char entryType; // file or dir 	
	unsigned int ownerId;
	unsigned int groupId;
	unsigned short int ownerPerm;
	unsigned short int groupPerm;
	unsigned short int publicPerm;
	unsigned int forwardPtr; // will point to inode of the file
	unsigned int size; // filesize in bytes // Not total rec size
	
	void initDirEntry(char _entryType, char *_fileName, struct UserInfo _uinfo )
	{
		memcpy(fileName, _fileName,MAX_FILE_NAME);	
		entryType = _entryType;
		ownerId = _uinfo.ownerId;
		groupId = _uinfo.groupId;
		ownerPerm = _uinfo.ownerPerm;
		groupPerm = _uinfo.groupPerm;
	}
	
}

struct Dir
{
	unsigned int reversePtr; // will point to parent directory
	unsigned int chainLink; // ptr to the next block of the same directory
	unsigned int revChainLink; // ptr to prev block of the same directory
	unsigned int numEntries; // number of entries in "this" block of the directory
}


bool existsFileName(char *newDirName, char *buf , int *entryIndex)
{
	struct Dir *header = (Dir*)buf;
	struct DirEntry *dirEntries = (DirEntry*)(&buf[sizeof(Dir)]); 
	for( int i = 0 ; i < header->numEntries ; i++)
	{
		if(strcmp(dirEntries[i].fileName,newDirName) == 0)
		{
			*entryIndex = i;	
			return true;
		}	
	}
	return false;
}

/* Creates a new dir if the dir does not exist, travesing the dir structure by the path is done earlier
   Assuming permission checking is done above
   TODO handle return codes */
int makeDir(char *newDirName, unsigned int parentDirBlock, struct UserInfo uinfo)
{	
	// Get parent directory's last page
	char *buf;
	unsigned int bufSize;
	struct Dir *header;
	int curBlock = parentDirBlock;
	int entryIndex = -1; // not used in this function
	while(true)
	{
		readDataIntoMem(curBlock,&buf, &bufSize);
		header = (Dir*) buf;
		if(existsFileName(newDirName,buf, &entryIndex)) // TODO:: return with appropriate return code;
		{
			freeDataFromMem(buf);
			return -1;
		}
		if(header->chainLink == 0) break;
		else curBlock = header->chainLink;
		freeDataFromMem(buf); // free space from Kernel RAM
	}
	
	// Calculate max entries in the parent's page
	unsigned int maxEntries = (bufSize - sizeof(Dir))/sizeof(DirEntry);
	
	// Create a new Directory entry
	struct DirEntry newEntry;
	newEntry.initDirEntry( 'd' , newDirName , uinfo);
	
	// Allocate a block for the new directory
	unsigned int newBlockNo;
	allocateBlock(&newBlockNo);
	newEntry.forwardPtr = newBlockNo;
	
	if( buf->numEntries < maxEntries)
	{
		// Entry can be added, page not full
		unsigned int offset = sizeof(Dir) + buf->numEntries*(sizeof(DirEntry));	
		memcpy(&buf[offset],&newEntry,sizeof (DirEntry));
		header->numEntries++;
	}
	else
	{
		// Current page for parent is full. We need to allocate a new page
		
		// Allocate new block
		unsigned int chainLinkBlockNo;
		allocateBlock(&chainLinkBlockNo);
		// Set link to new page of parent dir
		header->chainLink = chainLinkBlockNo;
		
		// Load new allocated block into memory
		char *newPage;
		readDataIntoMem(chainLinkBlockNo,&newPage, &bufSize);
		struct Dir *pageHeader = (Dir*) newPage;
		
		// Init page header
		pageHeader->numEntries=1;
		pageHeader->reversePtr = buf->reversePtr;
		pageHeader->chainLink = 0;
		pageHeader->revChainLink = curBlock;
		
		// Now write dir entry into this new page
		unsigned int offset = sizeof(Dir);	
		memcpy(&newPage[offset],&newEntry,sizeof (DirEntry));
		
		freeDataFromMem(newPage);
	}
	
	// Get new directory into memory
	char *newDirPage;
	readDataIntoMem(newBlockNo, &newDirPage, &bufSize);
	struct Dir *newDirHeader = (Dir*) newDirPage;
	newDirHeader->numEntries = 0;
	newDirHeader->reversePtr = parentDirBlock;
	newDirHeader->chainLink = 0;
	newDirHeader->revChainLink = 0;
	
	freeDataFromMem(newDirPage);
	freeDataFromMem(buf);
} 

int removeDir(char *dirName, unsigned int parentDirBlock)
{	
	// Get the page which contains the directory entry
	char *buf;
	unsigned int bufSize;
	struct Dir *header;
	int curBlock = parentDirBlock;
	int entryIndex = -1;
	while(true)
	{
		readDataIntoMem(curBlock,&buf, &bufSize);
		header = (Dir*) buf;
		if(existsFileName(dirName,buf,&entryIndex)) 
		{
			break;	
		}
		if(header->chainLink == 0)
		{
			freeDataFromMem(buf);
			// Dir not found, TODO:: return error	
			return -1;
		}
		else curBlock = header->chainLink;
		freeDataFromMem(buf); // free space from Kernel RAM
	}
	
	struct DirEntry *dirEntry = (DirEntry*)(&buf[sizeof(Dir)])[entryIndex];
	
	if(header->chainLink == 0)
	{	
		if(header->numEntries == 1)
		{
			if( header->revChainLink == 0)
			{
				// only file in directory, in this case only delete the entry	
				delRecursive(dirEntry->forwardPtr);
				
			}	
			
			
			
			
		}
	}	
}	


// TODO:: change param passed to freeDataFromMem
