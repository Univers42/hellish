/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 12:23:45 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* Free a glob token vector, including any char_set buffers owned by bracket
   tokens. The token array itself (tokens->ctx) is freed last. Safe to call
   with tokens->ctx == NULL (e.g. if glob_tokenize returned an empty vector).
   NULL-clears ctx and len so callers can safely call this multiple times. */
void	glob_free_tokens(t_vec_glob *tokens)
{
	size_t	i;
	t_glob	*g;

	if (!tokens || !tokens->ctx)
		return ;
	i = 0;
	while (i < tokens->len)
	{
		g = &((t_glob *)tokens->ctx)[i];
		if (g->char_set)
		{
			xfree(g->char_set);
			g->char_set = NULL;
		}
		i++;
	}
	xfree(tokens->ctx);
	tokens->ctx = NULL;
	tokens->len = 0;
}

/* Return true for the three POSIX glob metacharacters when unquoted.
   Note: '/' is intentionally excluded -- slashes are path separators, not
   wildcards; they're handled by the directory-walk structure, not matching. */
bool	glob_is_special(char c)
{
	return (c == '*' || c == '?' || c == '[');
}

/* Check whether `offset` is the last token in the current path segment.
   A segment ends when the next token is G_SLASH or there are no more tokens.
   Used by the pattern matchers to decide if a successful character match is
   also the end of the component -- without this check, "ab" would match "abc"
   because the literal match succeeds but there's still a 'c' left. */
bool	finished_pattern(t_vec_glob patt, size_t offset)
{
	t_glob	curr;

	if (offset + 1 >= patt.len)
		return (true);
	curr = ((t_glob *)patt.ctx)[offset + 1];
	if (curr.ty == G_SLASH)
		return (true);
	return (false);
}

/* Return the path for opendir. An empty string isn't a valid path on Linux
   (opendir("") returns ENOENT), so we substitute "." which always refers to
   the current directory. Relative glob patterns start with an empty path and
   need this substitution. */
char	*get_curr_path(char *path)
{
	if (*path)
		return (path);
	return (".");
}

/* Free one element of a char* vector. Takes a void* to match the vec_destroy
   callback signature; derefs the double-pointer and frees the string. */
void	free_str_elem(void *el)
{
	xfree(*(char **)el);
}
