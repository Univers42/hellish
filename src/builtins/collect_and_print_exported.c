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

/* collect exported entries into a vector of strings */
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

/* sort the list if needed */
static void	sort_export_list(t_vec *list)
{
	if (list->len > 1)
		ft_quicksort(list);
}

/* print and free the list of strings */
static void	print_and_free_list(t_vec *list)
{
	size_t	j;
	char	*s;

	j = 0;
	while (j < list->len)
	{
		s = ((char **)list->ctx)[j];
		ft_printf("%s\n", s);
		free(s);
		j++;
	}
	free(list->ctx);
}

/* public entry: collect, sort and print exported variables */
void	collect_and_print_exported(t_shell *st)
{
	t_vec	list;

	collect_exported_list(st, &list);
	sort_export_list(&list);
	print_and_free_list(&list);
}
