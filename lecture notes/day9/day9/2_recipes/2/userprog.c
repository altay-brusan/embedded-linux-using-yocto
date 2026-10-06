#include <stdio.h>

int main()
{
	#ifdef USE_SYSCALL
		write(1, "Hello System Call", 17);
	#else
		printf("Hello World");
	#endif
	return 0;
}
