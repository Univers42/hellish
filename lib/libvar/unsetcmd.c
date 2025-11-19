#include "libvar.h"
#include <string.h>

// Add external declaration for unsetfunc
extern void unsetfunc(const char *name);

static int parse_unset_options(int argc, char **argv, int *flag_out)
{
	int i;

	i = 1;
	*flag_out = 0;
	while (i < argc && argv[i][0] == '-')
	{
		if (strcmp(argv[i], "-v") == 0)
			*flag_out = 'v';
		else if (strcmp(argv[i], "-f") == 0)
			*flag_out = 'f';
		else if (strcmp(argv[i], "--") == 0)
		{
			i++;
			break;
		}
		else
			break;
		i++;
	}
	return (i);
}

static void process_unset_args(int argc, char **argv, int start_index, int flag)
{
	int i;

	i = start_index;
	while (i < argc)
	{
		if (flag != 'f')
		{
			unsetvar(argv[i]);
			if (flag == 'v')
			{
				i++;
				continue;
			}
		}
		if (flag != 'v')
			unsetfunc(argv[i]);
		i++;
	}
}

int unsetcmd(int argc, char **argv)
{
	int flag;
	int i;

	i = parse_unset_options(argc, argv, &flag);
	process_unset_args(argc, argv, i, flag);
	return (0);
}
