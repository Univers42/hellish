/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_text.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"

/* Trim trailing whitespace/newlines in place. */
static void	rstrip(char *s)
{
	size_t	i;

	i = ft_strlen(s);
	while (i > 0 && (s[i - 1] == '\n' || s[i - 1] == ' ' || s[i - 1] == '\t'))
		s[--i] = '\0';
}

/* If *sp points at a markdown code wrapper, advance *sp past the opener and set
   *ep to the closer (NULL if unterminated); return 1. Handles a ```lang fenced
   block and an inline `code` span. Returns 0 when there is no wrapper. */
static int	fence_inner(char **sp, char **ep)
{
	char	*p;

	p = *sp;
	if (!ft_strncmp(p, "```", 3))
	{
		p += 3;
		while (*p && *p != '\n')
			p++;
		if (*p == '\n')
			p++;
		*sp = p;
		return (*ep = ft_strnstr(p, "```", ft_strlen(p)), 1);
	}
	if (*p == '`')
		return (*sp = p + 1, *ep = ft_strchr(p + 1, '`'), 1);
	return (0);
}

/* Unwrap a markdown code wrapper to the bare command, in place. Models love
   wrapping commands in ``` fences or `inline` spans even when told not to; for
   `ai do` and inline completion we want the runnable command. No wrapper => the
   string is left untouched. */
void	ai_strip_fence(char *s)
{
	char	*start;
	char	*end;

	if (!s)
		return ;
	start = s;
	while (*start == ' ' || *start == '\n' || *start == '\t')
		start++;
	if (!fence_inner(&start, &end))
		return ;
	if (end)
		*end = '\0';
	ft_memmove(s, start, ft_strlen(start) + 1);
	rstrip(s);
}
