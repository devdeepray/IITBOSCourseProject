#include "virtualdisk.h"
#include "blocks.h"
#include "inode.h"

int main()
{
	int rc = Init_File_System();\
	fflush(stdout);
	printf("Format?\n");
	fflush(stdout);
	char mode;
	scanf("%c", &mode);
	if(mode == 'y')
	{
		rc = Format_Disk();
	fflush(stdout);
	}
	
	int i;
	for(i = 0; i <2; ++i)
	{
		fflush(stdout);
		int blocknum;
		Allocate_Block(&blocknum);
		printf("Allocated %d\n", blocknum);
		InodeMetaData inmd;
		strcpy(inmd.filename, "Filex");
		inmd.group_id = 1;
		inmd.owner_id = 2;
		inmd.permissions = 4;
		inmd.file_size = 0;
		int new_inode_num;
		Create_New_Inode(inmd, &new_inode_num);
		printf("Allocated inode %d\n", new_inode_num);
	}
		fflush(stdout);
	Shut_Down_File_System();
	
}
