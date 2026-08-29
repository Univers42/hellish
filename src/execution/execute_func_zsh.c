/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_func_zsh.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:50:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:50:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "brace_expand.h"

void	store_function(t_shell *state, char *name, t_ast_node *body,
			char *text);
char	*ast_source_text(t_ast_node *node);

/* One definition, several names.  Two zsh spellings reach here, and they
** compose -- oh-my-zsh uses both:
**
**     function man dman debman { ... }         a NAME LIST
**     function {pp,is,urlencode}_ndjson() { }  brace expansion in the name
**
** The parser hands over one span covering every name (parse_func_zsh.c);
** this splits it on whitespace and runs each piece through the ordinary
** brace expander, so the two features need no knowledge of each other.
**
** Each name gets its OWN body clone and its own source text.  Sharing one
** would be smaller and wrong: `unset -f man` must not take `debman` with
** it, and retire_body frees what a definition owns.
*/

/* Define `name`, brace-expanding it first.  `{a,b}_x` becomes two
   functions; a name with no braces expands to itself, so there is one path
   and not two. */
static void	zfunc_define(t_shell *state, const char *name, t_ast_node *body)
{
	t_vec	names;
	size_t	i;

	names = brace_expand_str(name);
	i = 0;
	while (i < names.len)
	{
		store_function(state, ((char **)names.ctx)[i], body,
			ast_source_text(body));
		xfree(((char **)names.ctx)[i++]);
	}
	xfree(names.ctx);
}

/* Split the name span on whitespace and define each.  Returns the number
   defined, so the caller can tell this apart from the ordinary one-name
   case without re-scanning. */
int	zfunc_define_all(t_shell *state, t_token *tok, t_ast_node *body)
{
	char	*name;
	int		start;
	int		i;
	int		n;

	i = 0;
	n = 0;
	while (i < tok->len)
	{
		while (i < tok->len && (tok->start[i] == ' ' || tok->start[i] == '\t'))
			i++;
		start = i;
		while (i < tok->len && tok->start[i] != ' ' && tok->start[i] != '\t')
			i++;
		if (i == start)
			continue ;
		name = ft_strndup(tok->start + start, (size_t)(i - start));
		zfunc_define(state, name, body);
		xfree(name);
		n++;
	}
	return (n);
}
