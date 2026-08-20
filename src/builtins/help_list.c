/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   help_list.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "help.h"
#include "libft.h"

int	get_cols(void);

/* Walk both halves of the index looking for an exact name. */
const t_help	*help_find(const char *name)
{
	const t_help	*e;
	int				half;

	half = 0;
	while (half < 2)
	{
		e = help_index();
		if (half == 1)
			e = help_index2();
		while (e->name)
		{
			if (ft_strcmp(e->name, name) == 0)
				return (e);
			e++;
		}
		half++;
	}
	return (NULL);
}

/* Print one entry as a row of the grouped listing: name, then the synopsis
   truncated to whatever the terminal leaves after the name column. */
static void	list_row(const t_help *e, int width)
{
	int	room;

	room = width - 14;
	if (room < 20)
		room = 20;
	ft_printf("  \033[1m%-10s\033[0m %.*s\n", e->name, room, e->synopsis);
}

/* Print every entry of one group, with the group as a heading. Called once
   per group name in help_print_list, which is what keeps the output in a
   deliberate order rather than the order the table happens to be in. */
static void	list_group(const char *group, int width)
{
	const t_help	*e;
	int				half;

	ft_printf("\n\033[1;38;5;209m%s\033[0m\n", group);
	half = 0;
	while (half < 2)
	{
		e = help_index();
		if (half == 1)
			e = help_index2();
		while (e->name)
		{
			if (ft_strcmp(e->group, group) == 0)
				list_row(e, width);
			e++;
		}
		half++;
	}
}

/* The no-argument listing. */
void	help_print_list(void)
{
	static const char	*order[] = {"navigation", "output", "variables",
		"jobs", "control", "commands", "tests", "history", "shell",
		"syntax", NULL};
	int					i;
	int					width;

	width = get_cols();
	ft_printf("\033[1mhellish\033[0m — these are built in. "
		"`help NAME` explains one, `help -s NAME` just its form.\n");
	i = 0;
	while (order[i])
		list_group(order[i++], width);
	ft_printf("\nEverything else on your $PATH works as usual; "
		"`type NAME` says which is which.\n");
}

/* One topic in full. -s prints only the form, which is what you want when
   you already know the command and just forgot an option. */
void	help_print_one(const t_help *e, int synopsis_only)
{
	if (synopsis_only)
		return ((void)ft_printf("%s\n", e->synopsis));
	ft_printf("\033[1m%s\033[0m  \033[2m(%s)\033[0m\n", e->name, e->group);
	ft_printf("  %s\n", e->synopsis);
	ft_printf("  %s\n", e->summary);
}
