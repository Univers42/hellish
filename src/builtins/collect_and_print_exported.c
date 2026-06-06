/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   collect_and_print_exported.c                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:45:29 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:55:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Walk the env table and format each exported entry as `export KEY="value"`.
   Variables without a value (exported but never assigned) use an empty
   string. All strings are heap-allocated — print_and_free_list owns them. */
static void	collect_exported_list(t_shell *st, t_vec *list)
{
	size_t	j;
	t_env	*e;
	char	*s;
	char	*tmp;

	vec_init(list);
	list->elem_size = sizeof(char *);
	j = 0;
	while (j < st->env.len)
	{
		e = &((t_env *)st->env.ctx)[j];
		if (e->exported)
		{
			tmp = "";
			if (e->value)
				tmp = e->value;
			s = ft_asprintf("export %s=\"%s\"", e->key, tmp);
			vec_push(list, &s);
		}
		j++;
	}
}

/* Sort the export list alphabetically (only when there is more than one
   entry). bash sorts `export -p` output; we match that so diff-based tests
   that check `export -p` output are not order-sensitive. */
static void	sort_export_list(t_vec *list)
{
	if (list->len > 1)
		ft_quicksort(list);
}

/* Print each formatted string, then free it and the backing array. We free
   inside the loop so there is never a large amount of memory live at once. */
static void	print_and_free_list(t_vec *list)
{
	size_t	j;
	char	*s;

	j = 0;
	while (j < list->len)
	{
		s = ((char **)list->ctx)[j];
		ft_printf("%s\n", s);
		xfree(s);
		j++;
	}
	xfree(list->ctx);
}

/* Public entry called by `export` with no arguments (or `export -p`): collect
   all exported variables, sort them, print them, and free the scratch list. */
void	collect_and_print_exported(t_shell *st)
{
	t_vec	list;

	collect_exported_list(st, &list);
	sort_export_list(&list);
	print_and_free_list(&list);
}
