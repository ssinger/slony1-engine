/*  */

#include <stdlib.h>
#include <stdio.h>

int
main(int argc, const char *argv[])
{
	int			lower,
				upper;
	int			base,
				result;

	if (argc != 3)
	{
		printf("args: %d - random_number: lower_limit upper_limit\n", argc);
		exit(1);
	}
	sscanf(argv[1], "%d", &lower);
	sscanf(argv[2], "%d", &upper);

	srand(time(0));
	base = rand();
	result = (base % (upper - lower)) + lower;

	printf("%d\n", result);
}
