/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_for_zsh.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 21:45:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 21:45:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "env.h"

void	set_for_var(t_shell *state, char *name, char *val);

/* zsh's multi-variable for: `for a b (x 1 y 2)` binds a=x b=1, then
** a=y b=2.  The parser stores the names as ONE space-separated span (see
** parse_for_zsh.c) precisely so this is the only place that has to know
** there was ever more than one.
**
** A short final group leaves the leftover names EMPTY rather than wrapping
** or stopping early -- `for a b (x)` runs once with a=x and b="" -- which is
** zsh's behaviour and the only rule that keeps the iteration count a
** function of the list length alone.
*/

/* Number of names in the span. */
int	zfor_count(const char *names, int len)
{
	int	n;
	int	i;

	n = 0;
	i = 0;
	while (i < len)
	{
		while (i < len && names[i] == ' ')
			i++;
		if (i < len)
			n++;
		while (i < len && names[i] != ' ')
			i++;
	}
	return (n);
}

/* The nth name in the span, freshly allocated, or NULL past the end. */
static char	*zfor_nth(const char *names, int len, int nth)
{
	int	i;
	int	start;

	i = 0;
	while (i < len)
	{
		while (i < len && names[i] == ' ')
			i++;
		start = i;
		while (i < len && names[i] != ' ')
			i++;
		if (i > start && nth-- == 0)
			return (ft_strndup(names + start, (size_t)(i - start)));
	}
	return (NULL);
}

/* Bind every name for one turn: the kth name takes words[base + k], or ""
   once the list has run out. */
void	zfor_bind_row(t_shell *state, t_ast_node *node, t_vec *w, size_t base)
{
	char	*name;
	int		n;
	int		k;

	n = zfor_count(node->token.start, node->token.len);
	k = -1;
	while (++k < n)
	{
		name = zfor_nth(node->token.start, node->token.len, k);
		if (!name)
			continue ;
		if (base + (size_t)k < w->len)
			set_for_var(state, name, ((char **)w->ctx)[base + k]);
		else
			set_for_var(state, name, "");
		xfree(name);
	}
}

/* How many words one turn of the loop consumes: 1 for every POSIX loop, and
   the number of NAMES for zsh's multi-variable form.  Derived from the name
   span rather than a flag, so a one-name loop takes the identical path it
   always did. */
size_t	for_stride(t_ast_node *node)
{
	int	n;

	n = zfor_count(node->token.start, node->token.len);
	if (n < 1)
		return (1);
	return ((size_t)n);
}
