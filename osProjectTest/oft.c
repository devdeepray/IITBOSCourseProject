#include <oft.h>

HashTable oft_hash;

typedef struct Global_Fcb {
int inodeNo;	// Should be fixed in the cache
int isWriting;
Inode* inode;
int numReading;
} Global_fcb;

int Init_Global_Fcb(Global_fcb *fcb, int inodeNo) {
if (!fcb) return -1;
fcb->inodeNo = inodeNo;
fcb->isWriting = 0;
fcb->numReading = 0;
return 0;
}

// The inode will always be in the cache
int Check_Perms(char op, Inode *buf) {
InodeMetaData metaData = buf->meta_data;

int ownerPerm = metaData->permissions % 8;
int groupPerm = (metaData->permissions / 8) % 8;
int publicPerm = ((metaData->permissions / 8) / 8) % 8;
// Check public perms
if (op == 'r') {
if (publicPerm >= 1) return 0;
if (CURRENT_THREAD->groupId == metaData->group_id && groupPerm >= 1) return 0;
if (CURRENT_THREAD->userId == metaData->owner_id && userPerm >= 1) return 0;
}
else (op == 'w' || op == 'a') {
if (publicPerm >= 3) return 0;
if (CURRENT_THREAD->groupId == metaData->group_id && groupPerm >= 3) return 0;
if (CURRENT_THREAD->userId == metaData->owner_id && userPerm >= 3) return 0;
}
return -1;
}

// Creates a new Global_fcb if it does not exist. Otherwise updates existing fcb
int Open_Oft(int inodeNo, char op, Inode **inode) {
// Check if already present in OFT
int firstTime = 0;
Global_Fcb *curFcb;
if (Get_From_Hash_Table(&oft_hash, inodeNo, &((void*)curFcb)) != -1) {
// Do nothing. here we have the inode in cache already 
}
// Need to create new fcb, get the inode into cache, set the inode pointer in curFcb
else {
Inode *temp_inode;
Global_Fcb *newFcb = Malloc(sizeof(Global_Fcb));
Get_Inode_Into_Cache(&lft_hash, inodeNo, &temp_inode);
Init_Global_Fcb(newFcb, inodeNo, temp_inode);
Add_To_Hash_Table(&oft_hash, inodeNo, (void*)newFcb);
curFcb = newFcb;
firstTime = 1;
}

// Check perms
Global_Fcb *curFcb;
int rc = Check_Perms(op, curFcb->inode);
if (rc == -1 && firstTime) {
Unfix_Inode_From_Cache(inodeNo);
Free(curFcb);
Remove_From_Hash_Table(&oft_hash, inodeNo);
return -1;
}

// Check permissions, update info
if (op == 'r') {
if (curFcb->isWriting) {
return -1;
}
else {
curFcb->numReading++;
}
}
else if (op == 'w' || op == 'a') {
if (curFcb->isWriting || curFcb->numReading) {
return -1;
}
else {
curFcb->isWriting = 1;
}
}
*inode = curFcb->inode;
return 0;
}

int Close_Oft(int inodeNo, char op) {
//Present in OFT
Global_Fcb *curFcb;
if (Get_From_Hash_Table(&oft_hash, inodeNo, &curFcb) != -1) {
// Do nothing here
}
// Not present in oft; give error
else {
return -1;
}

// Check op, update info
if (op == 'r') {
if (curFcb->numReading == 0) {
return -1;
}
else {
curFcb->numReading--;
}
}
else if (op == 'w') {
curFcb->isWriting = 0;
}
else if(op == 'a') {
curFcb->isWriting = 0;
Set_Inode_Dirty(inodeNo);
}
 
// If no proc is using the file, delete the entry, unfix the inode
if (curFcb->numReading == 0 && curFcb->isWriting == 0) {
Remove_From_Hash_Table(&oft_hash, inodeNo);
Unfix_Inode_From_Cache(inodeNo);
Free(curFcb);
}
return 0;
}

int Init_Oft() {
Init_Hash_Table(Hashtable &oft_hash, OFT_HASH_SIZE, OFT_HASH_MULT);
return 0;
}

/*************************************************************************************************/

enum SeekStart {F_START, F_CURR, F_END}

int fdCount;
HashTable lft_hash;

typedef Local_Fcb {
int seekPos;
char intendedOp;
int inodeNo;
Inode *inode;
} Local_Fcb;

int Init_Local_Fcb(LocalFcb *fcb, int inodeNo, char op, Inode *inode) {
fcb->seekPos = 0;
fcb->intendedOp = op;
fcb->inodeNo = inodeNo;
fcb->inode = inode;
}

int Get_Next_Fd(int *nextFd) {
void **dummy;
while(1) {
if (Get_From_Hash_Table(&lft_hash, fdCount, dummy) == -1) {
*nextFd = fdCount;
return 0;
}
fdCount++;
}
return -1;
}

int Init_Lft() {
Init_Hash_Table(Hashtable &lft_hash, LFT_HASH_SIZE, LFT_HASH_MULT);
fdCount = 0;
return 0;
}

int Open_Lft(int inodeNo, char op, int *fd) {
Inode *inode;
int oftRc = Open_Oft(inodeNo, op, &inode);
// error if perms don't match or file is being used
if (oftRc == -1) return -1;

int newFd;
Get_Next_Fd(&newFd);
Local_Fcb *newFcb = Malloc(sizeof(Local_Fcb));
Init_Local_Fcb(newFcb, inodeNo, op, inode);
Add_To_Hash_Table(&lft_hash, newFd, (void*)newFcb);
*fd = newFd;
return 0;
}

int Seek_Lft(int fileDes, int byteLoc, SeekStart pos) {
Local_Fcb *curFcb;
if (Get_From_Hash_Table(&lft_hash, fileDes, &((void*)curFcb)) == -1) return -1;

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

if (finalPos < 0 || finalPos >= fileSize) return -1;
curFcb->seekPos = finalPos;
return 0;
}

int Resize_Lft(int fd, int newSize) {
Local_Fcb *curFcb;
int rc = Check_Resize(fd);
if(!rc) return rc;
Get_From_Hash_Table(&lft_hash, fd, &((void*)curFcb));
int fileSize = curFcb->inode->meta_data->file_size;

int rc = 0;
if (fileSize > newSize) {
rc = Allocate_Upto(curFcb->inodeNo, newSize);
}
else if (fileSize < newSize) {
rc = Truncate_From(curFcb->inodeNo, newSize)
}
if (!rc) {
curFcb->inode->meta_data.file_size = newSize;
Set_Inode_Dirty(curFcb->inodeNo);
return 0; 
}
return -1;
}

int Check_Read(int fileDes, int nbytes) {
Local_Fcb *curFcb;
if (Get_From_Hash_Table(&lft_hash, fileDes, &((void*)curFcb)) == -1) return -1; 
if (curFcb->seekPos + nbytes > curFcb->fileSize) return -1;
return 0;	// If the file is open, you can read it.
}

int Check_Write(int fileDes, int nbytes) {
Local_Fcb *curFcb;
if (Get_From_Hash_Table(&lft_hash, fileDes, &((void*)curFcb)) == -1) return -1;
if (curFcb->seekPos + nbytes > curFcb->fileSize) return -1;
if (curFcb->intendedOp == 'w' || curFcb->intendedOp == 'a') return 0;
else return -1;
}

int Check_Resize(int fileDes){
Local_Fcb *curFcb;
if(Get_From_Hash_Table(&lft_hash,fileDes,&((void*)curFcb)) == -1) return -1;
if(curFcb->intendedOp == 'w' || curFcb->intendedOp == 'a') return 0;
else return -1;
}

int Close_Lft(int fileDes)
{
// check if entry actually
Local_Fcb *curFcb;
if (Get_From_Hash_Table(&lft_hash, fileDes, &((void*)curFcb)) == -1) return -1; 
int rc = Close_Oft(curFcb->inodeNo, curFcb->intendedOp);
if (rc) return -1;
Remove_From_Hash_Table(&lft_hash, fileDes);
Free(curFcb);
return 0;
}

int Get_Inode_Lft(int fd, Inode **inode) {
Local_Fcb *curFcb;
int rc = Get_From_Hash_Table(&lft_hash, fd, &((void*)curFcb));
*inode = curFcb->inode;
return rc;
}

int Get_Seek_Pos_Lft(int fd, int *seekPos) {
Local_Fcb *curFcb;
int rc = Get_From_Hash_Table(&lft_hash, fd, &((void*)curFcb));
*seekPos = curFcb->seekPos;
return rc;
}

int Get_File_Size_Lft(int fd, int *fileSize) {
Local_Fcb *curFcb;
int rc = Get_From_Hash_Table(&lft_hash, fd, &((void*)curFcb));
*file_size = curFcb->inode->meta_data->file_size;
return rc;
}

/************************************************************************************************/

int Init_Inode_Metadata(InodeMetaData *md, char* path) {
if (!path) return -1;
Path new_path(path);
while (path->childPath != NULL) {
path = path->childPath;
}
md.filename = path->fname;
md.user_id = CURRENT_THREAD->userId;
md.group_id = CURRENT_THREAD->groupId;
md.permissions = 115; //Default OsX perms :P
md.file_size = 0;
return 0;
}

int Open(char* path, char op, int* fd) {
int inodeNo;
int rc = Get_Inode_Num(path, &inodeNo);
if (rc == FILE_NOT_FOUND) {
//File does not exist here
int inodeNo;
Allocate_Inode(inodeNo);
InodeMetaData new_metadata;
Init_Inode_Metadata(&new_metadata, path);
Inode *new_inode = Malloc(sizeof(Inode));
Get_Inode_Into_Cache(inodeNo, &new_inode);
new_inode->meta_data = new_metadata;
new_inode->s_nest_ptr = NULL;
new_inode->d_nest_ptr = NULL;
Set_Inode_Dirty(inodeNo);
Unfix_Inode_From_Cache(inodeNo);
}
int rc = Open_Lft(inodeNo, op, fd);
return rc;
}

int Close(int fd) {
return Close_Lft(fd);
}

int Seek(int fd, int byteLoc, SeekStart pos) {
return Seek_Lft(fd, byteLoc, pos);
}

//does the reading when the exact info is available

int readLow(Inode* inode, char* buf, int seekPos, int nbytes) {
int toReadMore = nbytes;
int toReadFromFirst;
if (BLOCK_SIZE - (seekPos % BLOCK_SIZE) < nbytes) {
toReadFromFirst = BLOCK_SIZE - (seekPos % BLOCK_SIZE);
}
else {
toReadFromFirst = nbytes;
}

int blockNo;
Get_Block_For_Byte_Address(inode, seekPos, &blockNo);
char *curblock;
Get_Into_Cache(blockNo, &curblock);

memcpy((void*)buf, (void*)(&curblock[(seekPos % BLOCK_SIZE)]), toReadFromFirst); 
Unfix_From_Cache(blockNo);
//Done reading from first block

toReadMore -= toReadFromFirst;
if (toReadMore <= 0) {
return 0;
}

int firstByteToReadInBlock = seekPos + toReadFromFirst;
int numBlocksToRead = (toReadMore - 1) / BLOCK_SIZE + 1;
int bytesRead = toReadFromFirst;
int i;
for (i = 0; i < numBlocksToRead - 1; ++i) {
Get_Block_For_Byte_Address(inode, firstByteToReadInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)(&buf[bytesRead]), (void*)curblock, BLOCK_SIZE);
Unfix_From_Cache(blockNo);
bytesRead += BLOCK_SIZE;
firstByteToReadInBlock += BLOCK_SIZE;
toReadMore -= BLOCK_SIZE;
}

Get_Block_For_Byte_Address(inode, firstByteToReadInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)(&buf[bytesRead]), (void*)curblock, toReadMore);
Unfix_From_Cache(blockNo);
return 0;
}

// buf should be preallocated
int Read(int fd, char* buf, int nbytes) {
int rc = Check_Read(fd, nbytes);
rc = rc | (buf == NULL);
if (rc) return -1;
if (nbytes == 0) return 0;

Inode *inode;
Get_Inode_Lft(fd, &inode);


int seekPos;
Get_Seek_Pos_Lft(fd, &seekPos);

return readLow(indoe, buf, seekPos, nbytes)

/*
int toReadMore = nbytes;

// Read from first block
int toReadFromFirst;
if (BLOCK_SIZE - (seekPos % BLOCK_SIZE) < nbytes) {
toReadFromFirst = BLOCK_SIZE - (seekPos % BLOCK_SIZE);
}
else {
toReadFromFirst = nbytes;
}

int blockNo;
Get_Block_For_Byte_Address(inode, seekPos, &blockNo);
char *curblock;
Get_Into_Cache(blockNo, &curblock);

memcpy((void*)buf, (void*)(&curblock[(seekPos % BLOCK_SIZE)]), toReadFromFirst); 
Unfix_From_Cache(blockNo);
//Done reading from first block

toReadMore -= toReadFromFirst;
if (toReadMore <= 0) {
return 0;
}

int firstByteToReadInBlock = seekPos + toReadFromFirst;
int numBlocksToRead = (toReadMore - 1) / BLOCK_SIZE + 1;
int bytesRead = toReadFromFirst;
int i;
for (i = 0; i < numBlocksToRead - 1; ++i) {
Get_Block_For_Byte_Address(inode, firstByteToReadInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)(&buf[bytesRead]), (void*)curblock, BLOCK_SIZE);
Unfix_From_Cache(blockNo);
bytesRead += BLOCK_SIZE;
firstByteToReadInBlock += BLOCK_SIZE;
toReadMore -= BLOCK_SIZE;
}

Get_Block_For_Byte_Address(inode, firstByteToReadInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)(&buf[bytesRead]), (void*)curblock, toReadMore);
Unfix_From_Cache(blockNo);
return 0;*/
}

int writeLow(Inode* inode, const char* buf, int seekPos, int nbytes) {
int toWriteToFirst;
if (BLOCK_SIZE - (seekPos % BLOCK_SIZE) < nbytes) {
toWriteToFirst = BLOCK_SIZE - (seekPos % BLOCK_SIZE);
}
else {
toWriteToFirst = nbytes;
}

int blockNo;
Get_Block_For_Byte_Address(inode, seekPos, &blockNo);
char *curblock;
Get_Into_Cache(blockNo, &curblock);

memcpy((void*)(&curblock[(seekPos % BLOCK_SIZE)]), (void*)buf, toWriteToFirst);
Set_Dirty(blockNo);
Unfix_From_Cache(blockNo);
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
Get_Block_For_Byte_Address(inode, firstByteToWriteInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)curblock, (void*)(&buf[bytesWritten]), BLOCK_SIZE);
Set_Dirty(blockNo);
Unfix_From_Cache(blockNo);
bytesWritten += BLOCK_SIZE;
firstByteToWriteInBlock += BLOCK_SIZE;
toWriteMore -= BLOCK_SIZE;
}

Get_Block_For_Byte_Address(inode, firstByteToWriteInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)curblock, (void*)(&buf[bytesWritten]), toWriteMore);
Set_Dirty(blockNo);
Unfix_From_Cache(blockNo);
return 0;
}

int Write(int fd, const char* buf, int nbytes) {
int rc = Check_Write(fd, nbytes);
rc = rc | (buf == NULL);
if (rc) return -1;
if (nbytes == 0) return 0;

Inode *inode;
Get_Inode_Lft(fd, &inode);


int seekPos;
Get_Seek_Pos_Lft(fd, &seekPos);

return writeLow(inode,buf, seekPos, nbytes);

/*
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
char *curblock;
Get_Into_Cache(blockNo, &curblock);

memcpy((void*)(&curblock[(seekPos % BLOCK_SIZE)]), (void*)buf, toWriteToFirst);
Set_Dirty(blockNo);
Unfix_From_Cache(blockNo);
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
Get_Block_For_Byte_Address(inode, firstByteToWriteInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)curblock, (void*)(&buf[bytesWritten]), BLOCK_SIZE);
Set_Dirty(blockNo);
Unfix_From_Cache(blockNo);
bytesWritten += BLOCK_SIZE;
firstByteToWriteInBlock += BLOCK_SIZE;
toWriteMore -= BLOCK_SIZE;
}

Get_Block_For_Byte_Address(inode, firstByteToWriteInBlock, &blockNo);
Get_Into_Cache(blockNo, &curblock);
memcpy((void*)curblock, (void*)(&buf[bytesWritten]), toWriteMore);
Set_Dirty(blockNo);
Unfix_From_Cache(blockNo);
return 0;
*/
}

int Close(int fd) {
return Close_Lft(fd);
}

int Resize(int fd, int newSize) {
return Resize_Lft(fd, newSize);
}
