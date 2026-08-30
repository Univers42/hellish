/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arith_zsh.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 13:18:34 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 18:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"
#include "ft_glob.h"

/* zsh's `$+name[key]` inside arithmetic -- `(( $+commands[pixz] ))`, which
   is how every zsh plugin tests for a program and the last thing between
   oh-my-zsh's extract plugin and loading.
**
** Claimed as ONE variable token spanning the whole shape, `+` and subscript
** included, and resolved by get_var_value where a t_shell is in reach; the
** lexer has none, so it cannot check the dialect itself. That is safe in
** both dialects because this exact shape was a LEX ERROR before -- bash has
** no reading of it to preserve. Anything that is not the full shape falls
** through untouched.
*/
bool	lex_zsh_plus(t_arith_lexer *lex)
{
	int	start;
	int	i;

	if (lex->pos >= lex->len || lex->input[lex->pos] != '+')
		return (false);
	start = lex->pos;
	i = lex->pos + 1;
	if (i >= lex->len || !is_var_start(lex->input[i]))
		return (false);
	while (i < lex->len && is_var_char(lex->input[i]))
		i++;
	if (i >= lex->len || lex->input[i] != '[')
		return (false);
	while (i < lex->len && lex->input[i] != ']')
		i++;
	if (i >= lex->len)
		return (false);
	lex->pos = i + 1;
	lex->current.type = ATOK_VAR;
	lex->current.var_name = (char *)(lex->input + start);
	lex->current.var_len = lex->pos - start;
	lex->current.start = lex->input + start;
	lex->current.len = lex->pos - start;
	return (true);
}

/* zsh's `$#name` inside arithmetic -- `${a[$#a]}`, the last element, which
** is how a plugin pops a stack and the shape oh-my-zsh's dirhistory is built
** on.
**
** Claimed as ONE variable token with the `#` included, and resolved by
** get_var_value for the same reason lex_zsh_plus is: the lexer has no
** t_shell.  bash rejects this shape outright --
**
**     $ bash -c 'a=5; echo $(( $#a ))'
**     bash: 3a: value too great for base (error token is "3a")
**
** -- so no bash reading is being displaced.  Turning an ERROR into a number
** is still a change though, which is why this asks the dialect cell first;
** lex_zsh_plus could not, and that asymmetry is deliberate rather than an
** oversight, because `+name[key]` has no bash meaning to protect at all.
**
** A bare `$#` has no name after it and falls through untouched, so the
** positional count keeps working in both dialects.
*/
bool	lex_zsh_len(t_arith_lexer *lex)
{
	int	start;
	int	i;

	if (!glob_zsh() || lex->pos >= lex->len || lex->input[lex->pos] != '#')
		return (false);
	start = lex->pos;
	i = lex->pos + 1;
	if (i >= lex->len || !is_var_start(lex->input[i]))
		return (false);
	while (i < lex->len && is_var_char(lex->input[i]))
		i++;
	lex->pos = i;
	lex->current.type = ATOK_VAR;
	lex->current.var_name = (char *)(lex->input + start);
	lex->current.var_len = lex->pos - start;
	lex->current.start = lex->input + start;
	lex->current.len = lex->pos - start;
	return (true);
}
