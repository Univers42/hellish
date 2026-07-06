/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_join2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "history_private.h"
#include "libft.h"

/* Here-doc handling for the history joiner: collect the tags announced by
   << / <<- operators on the command line, then copy the body lines through
   literally until each terminator, so the recalled entry still contains a
   working here-doc (a "; "-joined body would never meet its terminator and
   wedge the shell waiting for it). */

/* Read the tag word after << (quotes stripped for matching, since <<'E'
   and <<E terminate on the same line). The stored tag is prefixed with '-'
   for <<- (tab-stripping) or '+' for <<, consumed by hj_heredoc_body. */
static void	hj_read_tag(t_hjoin *h, bool strip)
{
	t_string	tag;
	char		mark;
	char		*dup;

	vec_init(&tag);
	tag.elem_size = 1;
	mark = '+';
	if (strip)
		mark = '-';
	vec_push_char(&tag, mark);
	while (h->s[h->i] && !ft_strchr(" \t\n;&|<>()", h->s[h->i]))
	{
		if (h->s[h->i] != '\'' && h->s[h->i] != '"')
			vec_push_char(&tag, h->s[h->i]);
		vec_push_char(&h->out, h->s[h->i]);
		h->i++;
	}
	vec_ensure_space_n(&tag, 1);
	((char *)tag.ctx)[tag.len] = '\0';
	dup = (char *)tag.ctx;
	if (tag.len > 1)
		vec_push(&h->tags, &dup);
	else
		xfree(tag.ctx);
}

/* At a << that is not <<< (here-string): copy the operator through, note
   whether it strips tabs (<<-), skip the optional blanks, and queue the tag. */
void	hj_heredoc_tag(t_hjoin *h)
{
	bool	strip;

	vec_push_char(&h->out, '<');
	vec_push_char(&h->out, '<');
	h->i += 2;
	if (h->s[h->i] == '<')
	{
		vec_push_char(&h->out, '<');
		h->i++;
		return ;
	}
	strip = (h->s[h->i] == '-');
	if (strip)
	{
		vec_push_char(&h->out, '-');
		h->i++;
	}
	while (h->s[h->i] == ' ' || h->s[h->i] == '\t')
	{
		vec_push_char(&h->out, h->s[h->i]);
		h->i++;
	}
	hj_read_tag(h, strip);
}

/* Drop the front tag of the pending here-doc queue; leave body mode once
   every collected here-doc has met its terminator line. */
void	hj_pop_tag(t_hjoin *h)
{
	xfree(((char **)h->tags.ctx)[0]);
	ft_memmove(h->tags.ctx, (char **)h->tags.ctx + 1,
		(h->tags.len - 1) * sizeof(char *));
	h->tags.len--;
	h->body = (h->tags.len > 0);
}

/* True when the body line [s, s+n) is exactly the terminator tag. */
static bool	hj_line_matches(const char *s, size_t n, const char *tag)
{
	if (n != ft_strlen(tag))
		return (false);
	return (ft_strncmp(s, tag, n) == 0);
}

/* Copy one here-doc body line through literally (newline included) and pop
   the pending tag when the line is its terminator; <<- terminators may be
   indented with tabs. */
void	hj_heredoc_body(t_hjoin *h)
{
	size_t	end;
	size_t	start;
	char	*tag;

	end = h->i;
	while (h->s[end] && h->s[end] != '\n')
		end++;
	tag = ((char **)h->tags.ctx)[0];
	start = h->i;
	if (tag[0] == '-')
		while (h->s[start] == '\t')
			start++;
	if (hj_line_matches(h->s + start, end - start, tag + 1))
		hj_pop_tag(h);
	vec_push_nstr(&h->out, (char *)h->s + h->i, end - h->i);
	if (h->s[end] == '\n')
		vec_push_char(&h->out, '\n');
	h->i = end;
	if (h->s[end] == '\n')
		h->i++;
}
