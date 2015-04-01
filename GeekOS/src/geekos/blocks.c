#include <geekos/blocks.h>

superblock disk_superblock;

/*
	TODO this
		1) Handle all error codes
		2) Decide disk and file cache sizes
	TODO IOCS
		1) waitGetBlock - will definitely get a disk block (will block if needed) and fix it in the disk cache
		2) setDirtyBit - will set the dirty bit of the given blockNo in the disk cache (This has to be in the cache as this has not been yet unfixed)
		3) unFix - unfixes a block so that it can now be considered for eviction if needed.
		4) getBlockSizeInBytes()
		5) getTotBlocks()
	TODO inode
*/


int writeIntoBlock(int blockNo, char* data) {
	char* block;
	int rc = 0;
	rc = waitGetBlock(int blockNo, &block);
	if (rc) {
		return NOSUCHBLOCK;
	}
	memcpy((void*)block, data, disk_superblock.blockSizeInBytes);
	rc = setDirtyBit(blockNo);
	rc = unFixBlock(blockNo);
	return 0;
}

int readIntoMem(int blockNo, char* dataDest, int *blockSize) {
	char* block;
	int rc = getBlock(int blockNo, &block);
	memcpy((void*)dataDest, (void*)block, disk_superblock.blockSizeInBytes);
	unFixBlock(blockNo);
	*blockSize = disk_superblock.blockSizeInBytes;
	return 0;
}



/*
struct superblock {
	int blockSizeInBytes;
	int totBlocks;

	int diskCacheSize;
	int fileCacheSize;
	int numAllocatedBlocks;
	int firstFreeListBlock;
	int numFreeListBlocks;
	int firstInodeBlock;
	int numInodeBlocks;
};
*/


int initSuperBlock(superblock *disk_superblock) {
	disk_superblock->blockSizeInBytes = getBlockSizeInBytes();
	disk_superblock->totBlocks = getTotBlocks();
	disk_superblock->diskCacheSize;
	disk_superblock->fileCacheSize;

	disk_superblock->numFreeListBlocks = ((disk_superblock->totBlocks) / (disk_superblock->blockSizeInBytes * 8)) + 1;
	disk_superblock->numInodeBlocks = ((disk_superblock->totBlocks) / sizeof(inode)) + 1;
	disk_superblock->firstFreeListBlock = 1;
	disk_superblock->firstInodeBlock = disk_superblock->firstFreeListBlock + disk_superblock->numFreeListBlocks;
	disk_superblock->numAllocatedBlocks = 1 + disk_superblock->numInodeBlocks + disk_superblock->numFreeListBlocks;
}



int readSuperBlock(superblock *disk_superblock) {
	char* block;
	int rc = getBlock(0);
	memcpy((void*) disk_superblock, (void*) block, sizeof(superblock));
	unFixBlock(blockNo);
	return 0; 
}



/*
	bitwise data
	assumes that no data exists when this is called 
*/

int initFreeList() {
	int temp = 1;
	temp = temp << sizeof(unsigned char) * 8;
	unsigned char allOnes = temp - 1;


	int currBlockNo = disk_superblock.firstFreeListBlock;
	unsigned char* currBlock;
	int rc;
	int totUsed = disk_superblock.numAllocatedBlocks;
	int count = 0;
	for (; currBlockNo < disk_superblock.firstFreeListBlock + disk_superblock.numFreeListBlocks; ++currBlockNo) {
		rc = getBlock(currBlockNo, &currBlock);

		for (int i = 0; i < (disk_superblock.blockSizeInBytes) / sizeof(unsigned char); ++i, count += 8 * sizeof(unsigned char)) {
			if (count < totUsed) {
				if (count + sizeof(unsigned char) * 8 > totUsed) {
					int ones = count + sizeof(unsigned char) * 8 - totUsed;
					unsigned char someOnes = allOnes << (sizeof(unsigned char) * 8 - ones);
					currBlock[i] = someOnes;
				}
				else {
					currBlock[i] = allOnes;
				}
			}
			else {
				currBlock[i] = 0;
			}
		}
		setDirtyBit(currBlockNo);
		unFixBlock(currBlockNo);
	}
	return 0;
}


/* TODO allocate only after metadata blocks */
/* Data blocls are aligned to the sizeof(unsigned char) */
int allocateBlock(int* freeBlock) {
	int temp = 1;
	temp = temp << sizeof(unsigned char) * 8;
	unsigned char allOnes = temp - 1;

	*freeBlock = 0;
	int currBlockNo = disk_superblock.firstFreeListBlock;
	int rc;
	unsigned char* currBlock;
	int found = 0;
	int posInBlock = (disk_superblock.numAllocatedBlocks / sizeof(unsigned char)); 
	for (; currBlockNo < disk_superblock.firstFreeListBlock + disk_superblock.numFreeListBlocks; ++currBlockNo) {
		rc = getBlock(currBlockNo, &currBlock);

		for (posInBlock = 0; posInBlock < ((disk_superblock.blockSizeInBytes) * 8) / sizeof(unsigned char) * 8; ++posInBlock) {
			if (currBlock[posInBlock] == allOnes) {
				*freeBlock += sizeof(unsigned char) * 8;
			}
			else {
				found = 1;
				break;
			}
		}

		if (found) {
			unsigned char oneOne = 1;
			oneOne = oneOne << (sizeof(unsigned char) * 8 - 1);
			unsigned char currChar = currBlock[posInBlock];

			while(oneOne & currChar) {
				currChar << 1;
				*freeBlock++;
			}

			currBlock[posInBlock] = (currBlock[posInBlock] >> 1) | oneOne;

			disk_superblock.numAllocatedBlocks++;

			setDirtyBit(currBlockNo);
			unFixBlock(currBlockNo);
			return 0;
		}
		else {
			unFixBlock(currBlockNo);
		}
	}
	return -1;
}



int freeBlock(int blockNo) {
	int freeListBlockNo = disk_superblock.firstFreeListBlock + (blockNo / (disk_superblock.blockSizeInBytes * 8));

	unsigned char *freeListBlock;
	int rc = getBlock(freeListBlockNo, freeListBlock);

	blockNo -= (blockNo / (disk_superblock.blockSizeInBytes * 8)) * (disk_superblock.blockSizeInBytes * 8);

	if (freeListBlock[blockNo / sizeof(unsigned char)] >> (sizeof(unsigned char) - (blockNo % sizeof(unsigned char)) - 1) & 1) {	
		freeListBlock[blockNo / sizeof(unsigned char)] = freeListBlock[blockNo / sizeof(unsigned char)] << 1;
		setDirtyBit(currBlockNo);
		unFixBlock(currBlockNo);
		return 0;
	}
	else {
		unFixBlock(currBlockNo);
		return 0;
	}
}
