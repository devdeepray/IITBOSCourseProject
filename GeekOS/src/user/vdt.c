#include <virtdiskio.h>
#include <conio.h>
int main()
{
	Print("Seeking \n");
	Vir_Seek(100,20,1);
	Print("Seek over. Requesting IO\n");
	Vir_Read(100, NULL);
	Print("IO over.\n");
	return 0;
}

