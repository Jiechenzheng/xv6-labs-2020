#include <stdio.h>

int main(){
	printf("x=%d y=%d\n", 3);
	unsigned int i = 0x00646c72;
	printf("H%x Wo%s\n", 57616, &i);
}
