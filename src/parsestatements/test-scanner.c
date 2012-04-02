#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"

char		foo[65536];
extern int	STMTS[1024];
extern int	statements;

int
main(int argc, char *const argv[])
{

	int			i,
				j,
				START;
	int			nstatements = 0;

	fread(foo, sizeof(char), 65536, stdin);
	printf("Input: %s\n", foo);

	nstatements = scan_for_statements(foo);

	START = 0;
	for (i = 0; i < nstatements; i++)
	{
		printf("\nstatement %d\n-------------------------------------------\n", i);
		for (j = START; j < STMTS[i]; j++)
		{
			printf("%c", foo[j]);
		}
		START = STMTS[i];
	}

	return 0;
}
