#include <virtdiskio.h>
#include <conio.h>

int main(int argc, char** argv) {
	Print("Continuing will erase all the data on Sim Disk. Do you want to continue?");
	Get_Key();
	char key = Get_Key();
	if (key == 'Y' || key == 'y') {
		int rc = Vir_Format();
		if (rc) {
			Print("There was an error in formatting the disk.");
		}
		else {
			Print("Sim Disk has now been formatted.");
		}
		return 0;
	}
	else {
		return 0;
	}
}
