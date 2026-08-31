/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_func_zsh.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser_private.h"

/* `function a b c { ... }` -- one body, several names.
**
** oh-my-zsh's colored-man-pages is three lines of exactly this:
**
**     function man \
**       dman \
**       debman {
**       colored $0 "$@"
**     }
**
** and it is why `$0` inside the body matters: the three functions share a
** body and tell themselves apart by the name they were called under.
**
** The names are collected by WIDENING the name token to span them all, the
** same trick zfor_names uses for `for a b (...)`. They are adjacent in the
** source, so the span IS the text "man dman debman", and execute_func_def
** splits it. Extra AST children were the alternative and would have
** collided with the body, which already lives at children[0].
**
** Only in the zsh dialect, and only after the `function` keyword: bash has
** no such form, and `f() { }` cannot grow one because the `()` binds to a
** single name.
*/
void	zfunc_names(t_deque_tok *tokens, t_token *name)
{
	t_ltoken	*peek;

	peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	while (peek->tt == TT_WORD && !zfunc_is_cont(tokens->base, peek))
	{
		name->len = (int)(tokens->base + peek->off + peek->len
				- name->start);
		(void)deque_pop_start(&tokens->deqtok);
		peek = (t_ltoken *)deque_peek(&tokens->deqtok);
	}
}

/* A lone backslash: the line continuation in

       function man \
         dman \
         debman {

   survives as a WORD token, and collected as a name it defined a function
   literally called `\` alongside the three real ones. Not fatal -- nothing
   calls it -- which is exactly why it would have gone unnoticed; `declare
   -F` listing a backslash is the only place it shows. */
bool	zfunc_is_cont(const char *base, t_ltoken *t)
{
	return (t->len == 1 && base[t->off] == '\\');
}
