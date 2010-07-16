/*  */

#include <stdlib.h>
#include <stdio.h>

int
main(int argc, const char *argv[])
{
	int			length,
				i;
	int			base,
				result;

	if (argc != 2)
	{
		printf("args: %d - random_string: length\n", argc);
		exit(1);
	}
	sscanf(argv[1], "%d", &length);

	srand(time(0));
	for (i = 0; i < length; i++)
		printf("%c", (rand() % (122 - 48)) + 48);
	printf("\n", result);
}
