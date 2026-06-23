/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_models.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"

/* Count how many times pat occurs in buf. */
static int	count_key(const char *buf, const char *pat)
{
	int		c;
	char	*p;

	c = 0;
	p = ft_strnstr(buf, pat, ft_strlen(buf));
	while (p)
	{
		c++;
		p += 4;
		p = ft_strnstr(p, pat, ft_strlen(p));
	}
	return (c);
}

/* From p (at a "id" key) extract the following string value and advance *p.
   ponytail: model ids carry no escapes, so a plain copy to the next quote. */
static char	*grab_id(char **p)
{
	int	len;

	*p += 4;
	while (**p && **p != ':')
		(*p)++;
	while (**p && **p != '"')
		(*p)++;
	if (!**p)
		return (NULL);
	(*p)++;
	len = 0;
	while ((*p)[len] && (*p)[len] != '"')
		len++;
	return (*p += len, ft_substr(*p - len, 0, len));
}

/* GET /v1/models and collect every "id". Fresh array of fresh strings. */
char	**ai_models(int *count)
{
	char	*resp;
	char	**out;
	char	*p;
	int		i;

	*count = 0;
	resp = ai_get("/v1/models", 3000);
	if (!resp)
		return (NULL);
	out = xmalloc(sizeof(char *) * (count_key(resp, "\"id\"") + 1));
	p = ft_strnstr(resp, "\"id\"", ft_strlen(resp));
	i = 0;
	while (p)
	{
		out[i] = grab_id(&p);
		if (out[i])
			i++;
		p = ft_strnstr(p, "\"id\"", ft_strlen(p));
	}
	return (xfree(resp), *count = i, out);
}

void	ai_models_free(char **list, int n)
{
	int	i;

	if (!list)
		return ;
	i = 0;
	while (i < n)
		xfree(list[i++]);
	xfree(list);
}
