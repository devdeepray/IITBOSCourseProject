#include <geekos/oscourse/oft.h>
#include <geekos/oscourse/inode.h>

Hashtable oft_hash;



// Checks permissions for op allowed
int Check_Perms(int op, Inode *buf) {
	//~ Print("Entering check_perms\n");
	InodeMetaData metaData = buf->meta_data;

    //XXX Badal dalo
	int ownerPerm = metaData.permissions & 07;
	int groupPerm = (metaData.permissions & 070) >> 3;
	int publicPerm = (metaData.permissions & 0700) >> 6;
	//~ Print("%d %d %d\n", ownerPerm, groupPerm, publicPerm);
	//~ Print("op %d\n", op);
	// Check public perms
	if (op == 1) { // Read
		//~ Print("Camer here as well\n");
		if (publicPerm >= 1) {
			//~ Print("check perms \n");
			return 0;
		}
		if (CURRENT_THREAD->group_id == metaData.group_id && groupPerm >= 1) return 0;
		if (CURRENT_THREAD->user_id == metaData.owner_id && ownerPerm >= 1) return 0;
	}
	else if (op == 2 || op == 3) { // Write/append
		//~ Print("Came here\n");
		if (publicPerm >= 3) return 0;
		if (CURRENT_THREAD->group_id == metaData.group_id && groupPerm >= 3) return 0;
		if (CURRENT_THREAD->user_id == metaData.owner_id && ownerPerm >= 3) return 0;
	}
	Print("!!!USER NOT ALLOWED ACCESS. INCORRECT ACCESS DETECTED!!!\n");
	return -1;
}

// Creates a new Global_Fcb if it does not exist. Otherwise updates existing fcb
int Open_Oft(int inodeNo, int op) {
	//~ Print("Entering open_oft\n");
    //~ Print("Adding global fcb\n");
	Global_Fcb *curFcb;
	
	// Need to create new fcb, get the inode into cache, set the inode pointer in curFcb
	if(Get_From_Hash_Table(&oft_hash,inodeNo, (void**)&curFcb) == -1)
	{
		curFcb = (Global_Fcb*)Malloc(sizeof(Global_Fcb));
		curFcb->inodeNo = inodeNo;
	    curFcb->isWriting = 0;
	    curFcb->numReading = 0;
		Add_To_Hash_Table(&oft_hash, inodeNo, (void*)curFcb);
	}

	// Check permissions, update info

	if (op == 2 || op == 3) {
		if (curFcb->isWriting) {
		    Print("SOMETHING ELSE ALREADY WRITING");
			return -1;
		}
		else {
			curFcb->isWriting = 1;
		}
	}
	curFcb->numReading++;
	//~ Print("Added global FCB\n");
	return 0;
}

int Close_Oft(int inodeNo, int op) {
    //~ Print("Closing globally\n");
	//Present in OFT
	Global_Fcb *curFcb;
	if (Get_From_Hash_Table(&oft_hash, inodeNo, (void**)&curFcb) == -1) {
        Print("FILE NOT OPENED EARLIER\n");
        return -1;
	}

	// Check op, update info
	if (op == 1) {
		curFcb->numReading--;
	}
	else if (op == 2 || op == 3) {
		curFcb->isWriting = 0;
		curFcb->numReading--;
	}
	 
	// If no proc is using the file, delete the entry
	if (curFcb->numReading == 0 && curFcb->isWriting == 0) {
		Remove_From_Hash_Table(&oft_hash, inodeNo);
		Free(curFcb);
	}
	//Print("Closed globally\n");
	return 0;
}

// Initialize OFT structures
int Init_Oft() {
    //~ Print("Initializing OFT table\n");
	Init_Hash_Table(&oft_hash, OFT_HASH_SIZE, OFT_HASH_MULT);
    Print("Initialized OFT table\n");
	return 0;
}

/*************************************************************************************************/


int Init_Local_Fcb(Local_Fcb *fcb, int inodeNo, int op, Inode *inode) {
	//~ Print("Entering init_local_fcb\n");
	fcb->seekPos = 0;
	fcb->intendedOp = op;
	fcb->inodeNo = inodeNo;
	fcb->inode = inode;
	return 0;
}

int Get_Next_Fd(int *nextFd) {
	//~ Print("Entering get_next_fd\n");
	void *dummy;
	while(1) {
		if (Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), CURRENT_THREAD->fdCount, &dummy) == -1) {
			*nextFd = CURRENT_THREAD->fdCount;
			return 0;
		}
		CURRENT_THREAD->fdCount++;
	}
	return -1;
}

/// Call from kthread init
int Init_Lft(struct Kernel_Thread *kthread) {	
	Init_Hash_Table(&(kthread->lft_hash), LFT_HASH_SIZE, LFT_HASH_MULT);
	kthread->fdCount = 0;
	return 0;
}

int Open_Lft(int inodeNo, int op, int *fd) {
    //~ Print("Opening in local file table\n");
	Inode *inode;
	int rc = Get_Inode_Into_Cache(inodeNo, &inode);
	if(rc)
	{
	    Print("oft.c/Open_Lft: Could not get inode into cache\n");
	    return rc;
	}
	
	if(inode->meta_data.is_directory)
	{
		Print("Is directory, cannot open directory\n");
		Unfix_Inode_From_Cache(inodeNo);
		return -1;
	}
	
	// Check perms
	//~ Print("Open lft op is %d\n", op);
    rc = Check_Perms(op, inode);
    rc = rc | Open_Oft(inodeNo, op);
	
	if (rc == -1) {
	    Print("NOT ALLOWED TO OPEN FILE. INVALID ACCESS DETECTED\n");
		rc = Unfix_Inode_From_Cache(inodeNo);
		if(rc)
		{
		    Print("oft.c/Open_lft: FATAL: Could not unfix a fixed inode");
		}
		return -1;
	}

	int newFd;
	Get_Next_Fd(&newFd);
	Local_Fcb *newFcb = Malloc(sizeof(Local_Fcb));
	Init_Local_Fcb(newFcb, inodeNo, op, inode);
	rc = Add_To_Hash_Table(&(CURRENT_THREAD->lft_hash), newFd, (void*)newFcb);
	if(rc)
	{
	    Print("oft.c/Open_Lft: Could not add to hash table. FATAL error\n");
	    Close_Oft(inodeNo, op);
	    Free(newFcb);
	    rc = Unfix_Inode_From_Cache(inodeNo);
		if(rc)
		{
		    Print("oft.c/Open_lft: FATAL: Could not unfix a fixed inode");
		}
		return -1;
	}
	*fd = newFd;
    Print("Opened in local file table\n");
	return 0;
}

int Seek_Lft(int fileDes, int byteLoc, SeekStart pos) {
	Local_Fcb *curFcb;
	if (Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fileDes, (void**)&curFcb) == -1) {
	    Print("oft.c/Seek_Lft: FILE WAS NEVER OPENED.");
	    return -1;
	}

	int fileSize = ((curFcb->inode)->meta_data).file_size;
	int finalPos;
	switch(pos) {
		case F_START:
			finalPos = byteLoc;
			break;
		case F_CURR:
			finalPos = curFcb->seekPos + byteLoc;
			break;
		case F_END:
			finalPos = fileSize - byteLoc - 1;
			break;
	}

	if (finalPos < 0 || finalPos >= fileSize) {
	    Print("oft.c/Seek_Lft: Seeking beyond the file size.");
	    return -1;
	}
	curFcb->seekPos = finalPos;
	return 0;
}

int Resize_Lft(int fd, int newSize) {
	Local_Fcb *curFcb;
	int rc = Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fd, (void**)&curFcb);
	if (rc) {
		Print("oft.c/Resize_Lft: FILE WAS NEVER OPENED.");
		return -1;	
	}
	
	if (!(curFcb->intendedOp == 2 || curFcb->intendedOp == 3)) {
		Print("oft.c/Resize_Lft: File was not opened with resizing permissions.");
		return -1;
	}

	int fileSize = curFcb->inode->meta_data.file_size;

	rc = 0;
	if (fileSize < newSize) {
		rc = Allocate_Upto(curFcb->inodeNo, newSize);
		if(rc) {
	       Print("oft.c/Resize_Lft: File allocate failed.");
	       return -1;
	    }
	}
	else if (fileSize > newSize) {
		rc = Truncate_From(curFcb->inodeNo, newSize);
		if(rc) {
	       Print("oft.c/Resize_Lft: File truncate failed.");
	       return -1;
	    }
	}
	return 0;
}

int Close_Lft(int fileDes)
{
	// check if entry actually
	Local_Fcb *curFcb;
	if (Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fileDes, (void**)&curFcb) == -1) {
		Print("ift.c/Close_Lft: FILE WAS NEVER OPENED.");
		return -1; 
	}
	
	int rc = Close_Oft(curFcb->inodeNo, curFcb->intendedOp);
	if (rc) {
		Print("oft.c/Close_Lft: Could not close file in oft.");
		return -1;
	}
	rc = Unfix_Inode_From_Cache(curFcb->inodeNo);
	if (rc) {
		Print("oft.c/Close_Lft: Could not close file in oft.");
		return -1;
	}
	if (rc) return -1;
	Remove_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fileDes);
	Free(curFcb);
	return 0;
}

int Get_Seek_Pos_Lft(int fd, int *seekPos) {
	Local_Fcb *curFcb;
	int rc = Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fd, (void**)&curFcb);
	*seekPos = curFcb->seekPos;
	return rc;
}

int Get_File_Size_Lft(int fd, int *fileSize) {
	Local_Fcb *curFcb;
	int rc = Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fd, (void**)&curFcb);
	*fileSize = curFcb->inode->meta_data.file_size;
	return rc;
}

/************************************************************************************************/

int Init_Inode_Metadata(InodeMetaData *md, char* filename) {
	strcpy(md->filename, filename);
	md->owner_id = CURRENT_THREAD->user_id;
	md->group_id = CURRENT_THREAD->group_id;
	md->permissions = 0115; //Default OsX perms :P
	md->file_size = 0;
	md->is_directory = 0;
	return 0;
}

int My_Open(char* path,char* fname, int op, int* fd) {
	//~ Print("Myopen Op is %d\n", op);
	//~ Print("Hash size in oft: %d", inode_manager.cache_hash.size);
	//~ Print("Entering my_open\n");
	int inodeNo;
	Inode *pwdInode;
	Path pwd;
	Create_Path(path, &pwd);
	//~ Print("Child is %d\n", (int)(pwd.childPath));
	//~ Print("path created\n");
	int rc = Get_Inode_From_Path(pwd, &inodeNo);
	
	//~ Print("Inode num %d\n", inodeNo);
	//~ Print("get inode from path\n");
	if(rc) return rc;
	rc = Get_Inode_Into_Cache(inodeNo, &pwdInode);
	//~ Print("Inodename", inodeNo);
	Unfix_Inode_From_Cache(inodeNo);
	Print("root name %s\n", pwdInode->meta_data.filename);
	if (rc == -1) {
		Print("oft.c/My_Open: Could not get pwd inode \n");
		return -1;
	}
	
	int fileInodeNum, entryNum;
	//~ Print("Myopen Op is %d\n", op);
	rc = Exists_File_Name(pwdInode, fname, &fileInodeNum, &entryNum);
	Print("rc is %d", rc);
	//~ Print("Myopen Op is %d\n", op);
	if (rc) {
		//~ Print("HERE, NEWFILE %s\n", fname);
		//File does not exist here
		InodeMetaData new_metadata;
		Init_Inode_Metadata(&new_metadata, fname);
		rc = Create_New_Inode(new_metadata, &fileInodeNum);
		
		if(rc)
		{
			Print("Could not create inode for new file\n");
			return rc;
		}
		DirHeader dhead;
		rc = My_ReadLow(pwdInode, (char*) &dhead, 0, sizeof(DirHeader));
		if(rc)
		{
			Unfix_Inode_From_Cache(inodeNo);
			return rc;
		}
		
		dhead.numEntries++;
		rc = My_WriteLow(pwdInode, (char*)&dhead, 0, sizeof(DirHeader));
		if(rc) 
		{
			Unfix_Inode_From_Cache(inodeNo);
			return rc;
		}
		rc = Allocate_Upto(inodeNo, sizeof(DirEntry) + pwdInode->meta_data.file_size);
		if(rc)
		{
			Unfix_Inode_From_Cache(inodeNo);
			return rc;
		}
		struct DirEntry dentry;
		strcpy(dentry.fname, fname);
		dentry.inode_num = fileInodeNum;
		rc = My_WriteLow(pwdInode, (char*)&dentry, pwdInode->meta_data.file_size - sizeof(DirEntry), sizeof(DirEntry));
		
		if(rc) 
		{
			Unfix_Inode_From_Cache(inodeNo);
			return rc;
		}
	}
	//~ Print("Myopen Op is %d\n", op);
	rc = Open_Lft(fileInodeNum, op, fd);
	if (rc) {
		Print("oft.c/My_Open: Could not open file in lft.\n");
		return -1;
	}
	return 0;
}

int My_Close(int fd) {
	return Close_Lft(fd);
}

int My_Seek(int fd, int byteLoc, SeekStart pos) {
	return Seek_Lft(fd, byteLoc, pos);
}

//does the reading when the exact info is available

int My_ReadLow(Inode* inode, char* buf, int seekPos, int nbytes) {
	int toReadMore = nbytes;
	int toReadFromFirst;
	if (BLOCK_SIZE - (seekPos % BLOCK_SIZE) < nbytes) {
		toReadFromFirst = BLOCK_SIZE - (seekPos % BLOCK_SIZE);
	}
	else {
		toReadFromFirst = nbytes;
	}

	int blockNo;
	int rc = Get_Block_For_Byte_Address(inode, seekPos, &blockNo);
	//Print("Block addr %d\n", blockNo);
	if (rc) {
		Print("oft.c/My_ReadLow: Could not get block for byte address.");
		return -1;
	}
	char *curblock;
	rc = Get_Into_Cache(blockNo, &curblock);
	if (rc) {
		Print("oft.c/My_ReadLow: Could not get block into cache.\n");
		return -1;
	}
	
	memcpy((void*)buf, (void*)(&curblock[(seekPos % BLOCK_SIZE)]), toReadFromFirst); 
	//Print("buf: %s\n", buf);
	Unfix_From_Cache(blockNo);
	//Done reading from first block

	toReadMore -= toReadFromFirst;
	if (toReadMore <= 0) {
		return 0;
	}

	rc = 0;
	int firstByteToReadInBlock = seekPos + toReadFromFirst;
	int numBlocksToRead = (toReadMore - 1) / BLOCK_SIZE + 1;
	int bytesRead = toReadFromFirst;
	int i;
	for (i = 0; i < numBlocksToRead - 1; ++i) {
		rc = Get_Block_For_Byte_Address(inode, firstByteToReadInBlock, &blockNo);
		if (rc) {
			Print("oft.c/My_ReadLow: Could not get block for byte address.\n");
			return -1;
		}
		rc = Get_Into_Cache(blockNo, &curblock);
		if (rc) {
			Print("oft.c/My_ReadLow: Could not get block into cache.\n");
			return -1;
		}
		memcpy((void*)(&buf[bytesRead]), (void*)curblock, BLOCK_SIZE);
		rc = Unfix_From_Cache(blockNo);
		if (rc) {
			Print("oft.c/My_ReadLow: Could not get unfix block from cache.\n");
			return -1;
		}
		bytesRead += BLOCK_SIZE;
		firstByteToReadInBlock += BLOCK_SIZE;
		toReadMore -= BLOCK_SIZE;
	}

	rc = Get_Block_For_Byte_Address(inode, firstByteToReadInBlock, &blockNo);
	if (rc) {
		Print("oft.c/My_ReadLow: Could not get block for byte address.\n");
		return -1;
	}
	rc = Get_Into_Cache(blockNo, &curblock);
	if (rc) {
		Print("oft.c/My_ReadLow: Could not get block into cache.\n");
		return -1;
	}
	memcpy((void*)(&buf[bytesRead]), (void*)curblock, toReadMore);
	rc = Unfix_From_Cache(blockNo);
	if (rc) {
		Print("oft.c/My_ReadLow: Could not get unfix block from cache.\n");
		return -1;
	}
	return 0;
}

// buf should be preallocated
int My_Read(int fd, char* buf, int nbytes) {
	Local_Fcb *curFcb;
	int rc = Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fd, (void**)&curFcb);
	if (rc) {
		Print("oft.c/My_Read: FILE WAS NEVER OPENED.\n\n");
		return -1;
	}
	
	if (nbytes == 0) return 0;
	
	if (curFcb->seekPos + nbytes > curFcb->inode->meta_data.file_size) {
		Print("oft.c/My_Read: Read goes beyond the size of the file.\n");
		return -1;
	}

	return My_ReadLow(curFcb->inode, buf, curFcb->seekPos, nbytes);


}

int My_WriteLow(Inode* inode, const char* buf, int seekPos, int nbytes) {
	int toWriteMore = nbytes;
	// Write to first block
	int toWriteToFirst;
	if (BLOCK_SIZE - (seekPos % BLOCK_SIZE) < nbytes) {
		toWriteToFirst = BLOCK_SIZE - (seekPos % BLOCK_SIZE);
	}
	else {
		toWriteToFirst = nbytes;
	}

	int blockNo;
	Get_Block_For_Byte_Address(inode, seekPos, &blockNo);
	
	Print("%d block\n", blockNo);
	char *curblock;
	Get_Into_Cache(blockNo, &curblock);

	memcpy((void*)(&curblock[(seekPos % BLOCK_SIZE)]), (void*)buf, toWriteToFirst);
	int rc = Set_Dirty(blockNo);
	if (rc) {
		Print("oft.c/My_WriteLow: Could not set block dirty.\n");
		return -1;
	}
	rc = Unfix_From_Cache(blockNo);
	if (rc) {
		Print("oft.c/My_WriteLow: Could not set block dirty.\n");
		return -1;
	}
	//Done writing to first block

	toWriteMore -= toWriteToFirst;
	if (toWriteMore <= 0) {
		return 0;
	}

	int firstByteToWriteInBlock = seekPos + toWriteToFirst;
	int numBlocksToWriteTo = (toWriteMore - 1) / BLOCK_SIZE + 1;
	int bytesWritten = toWriteToFirst;
	int i;
	for (i = 0; i < numBlocksToWriteTo - 1; ++i) {
		rc = Get_Block_For_Byte_Address(inode, firstByteToWriteInBlock, &blockNo);
		if (rc) {
			Print("oft.c/My_WriteLow: Could not get bloack for byte.\n");
			return -1;
		}
		rc = Get_Into_Cache(blockNo, &curblock);
		if (rc) {
			Print("oft.c/My_WriteLow: Could not get block into cache.\n");
			return -1;
		}
		memcpy((void*)curblock, (void*)(&buf[bytesWritten]), BLOCK_SIZE);
		rc = Set_Dirty(blockNo);
		if (rc) {
			Print("oft.c/My_WriteLow: Could not set block dirty.\n");
			return -1;
		}
		rc = Unfix_From_Cache(blockNo);
		if (rc) {
			Print("oft.c/My_WriteLow: Could not set block dirty.\n");
			return -1;
		}
		bytesWritten += BLOCK_SIZE;
		firstByteToWriteInBlock += BLOCK_SIZE;
		toWriteMore -= BLOCK_SIZE;
	}

	rc = Get_Block_For_Byte_Address(inode, firstByteToWriteInBlock, &blockNo);
	if (rc) {
		Print("oft.c/My_WriteLow: Could not get bloack for byte.\n");
		return -1;
	}
	rc = Get_Into_Cache(blockNo, &curblock);
	if (rc) {
		Print("oft.c/My_WriteLow: Could not get block into cache.\n");
		return -1;
	}
	memcpy((void*)curblock, (void*)(&buf[bytesWritten]), toWriteMore);
	rc = Set_Dirty(blockNo);
	if (rc) {
		Print("oft.c/My_WriteLow: Could not set block dirty.\n");
		return -1;
	}
	rc = Unfix_From_Cache(blockNo);
	if (rc) {
		Print("oft.c/My_WriteLow: Could not set block dirty.\n");
		return -1;
	}
	return 0;
}

int My_Write(int fd, const char* buf, int nbytes) {
	
	Local_Fcb *curFcb;
	int rc = Get_From_Hash_Table(&(CURRENT_THREAD->lft_hash), fd, (void**)&curFcb);
	//~ Print("%d sk, %d nbytes\n", curFcb->seekPos, nbytes);
	if (rc) {
		Print("oft.c/My_Read: FILE WAS NEVER OPENED.\n");
		return -1;
	}

	if (nbytes == 0) return 0;
	
	if (curFcb->seekPos + nbytes > curFcb->inode->meta_data.file_size) {
		Print("oft.c/My_Write: Write goes beyond the size of the file.\n");
		return -1;
	}
	if (!(curFcb->intendedOp == 2 || curFcb->intendedOp == 3)) {
		Print("oft.c/My_Write: File was not opened for writing.\n");
		return -1;
	}

	return My_WriteLow(curFcb->inode,buf, curFcb->seekPos, nbytes);

	
}

int My_Resize(int fd, int newSize) {
	return Resize_Lft(fd, newSize);
}
