#include <virtfileio.h>
#include <conio.h>

int main() {
	int i;
	int start = 25;
	
	for (i = 0; i < 35; i++){
		Vir_Get_Into_Cache(start + i);
		Vir_Unfix_From_Cache(start + i);
	}
	/*
	for (i = 0; i < 64; i++){
		Print("Fixing %d)",i);
		Vir_Get_Into_Cache(start + i);
	}
	for (i = 0; i< 10; i++){
		Vir_Unfix_From_Cache(start + i);
	}
	for (i = 0; i < 10; i++){
		Print("Refixing %d)",i);
		Vir_Get_Into_Cache(start + i + 20);
	}
	* */
	return 0;
}
	

