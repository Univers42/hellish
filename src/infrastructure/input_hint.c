/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_hint.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "lexer.h"
#include "sh_error.h"
#include "prompt.h"

/* "unexpected end of file" that names the cure -- issue #112.
**
** A 42 student pasted a ~/.zshrc into ~/.hellishrc. Line 3 was
**
**     precmd() { vcs_info }
**
** which is a function in zsh, where a bare `}` ALWAYS closes the group, and
** an open group in bash, where `}` is a reserved word only at the start of
** a command -- here it is an argument to vcs_info. Bash then swallowed the
** rest of the file and reported only "syntax error: unexpected end of
** file", pointing at nothing. The bash dialect keeps bash's answer (`echo
** }` prints a brace, and the golden suite pins it); what changes is the
** diagnosis. When the unterminated construct contains a bare `}` word
** inside an open brace group, the message says which line, why, and the
** two ways out: `emulate zsh` at the top, or a .zsh extension under rc.d.
**
** The `}` is remembered by the lexer (brace_step -> zsh_brace_cell) rather
** than found here: the parser pops tokens as it consumes them, so at the
** moment the error is reported the deque holds nothing. brace_step counts
** depth from the promoted TT_LBRACE / TT_RBRACE tokens, so a `{` that
** stayed a word cannot open a group and cannot trigger this.
*/
static int	brace_line(t_deque_tok *tt)
{
	long	off;

	off = *zsh_brace_cell();
	if (off < 0 || (size_t)off >= ft_strlen(tt->base))
		return (0);
	return (nl_count(tt->base, (size_t)off) + 1);
}

void	zsh_brace_hint(t_shell *state, t_deque_tok *tt)
{
	int	line;

	if (!tt || !tt->base || zsh_mode(state))
		return ;
	line = brace_line(tt);
	if (line <= 0)
		return ;
	ft_eprintf("%s: line %d: a bare `}' closes a group only in zsh; "
		"in bash it is an argument, and the group stayed open\n",
		state->ctx, line);
	ft_eprintf("%s: hint: this looks like zsh -- start the file with "
		"`emulate zsh', or save it as ~/.config/hellish/rc.d/NN-name.zsh\n",
		state->ctx);
}
