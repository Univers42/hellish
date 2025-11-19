#include "libvar.h"
#include <string.h>

static void	process_export_arg(char *arg, int flag)
{
	struct s_var	*vp;
	const char		*p;

	p = strchr(arg, '=');
	if (p != NULL)
	{
		p++;
		setvar(arg, p, flag);
	}
	else
	{
		vp = *var_find(var_hash(arg), arg);
		if (vp)
			vp->flags |= flag;
		else
			setvar(arg, NULL, flag);
	}
}

static int	parse_export_options(int argc, char **argv, int *show_p)
{
	int	i;

	i = 1;
	*show_p = 0;
	while (i < argc)
	{
		if (argv[i][0] != '-')
			break ;
		if (strcmp(argv[i], "-p") == 0)
			*show_p = 1;
		else if (strcmp(argv[i], "--") == 0)
		{
			i++;
			break ;
		}
		i++;
	}
	return (i);
}

int	exportcmd(int argc, char **argv)
{
	int	flag;
	int	show_p;
	int	i;

	if (argv[0][0] == 'r')
		flag = VREADONLY;
	else
		flag = VEXPORT;
	i = parse_export_options(argc, argv, &show_p);
	if (show_p || i == argc)
	{
		showvars(argv[0], flag, 0);
	}
	else
	{
		while (i < argc)
		{
			process_export_arg(argv[i], flag);
			i++;
		}
	}
	return (0);
}
