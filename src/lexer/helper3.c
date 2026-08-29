/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper3.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:00:38 by marvin            #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "ft_glob.h"

/* Operator recognition used to walk a 21-entry table rebuilt on the stack
   at every call, with ~60 ft_strlen/ft_strncmp calls per operator token —
   measurable in the parse profile. It is now a first-character dispatch:
   the leading byte picks one of three tiny matchers doing direct char
   compares, longest form first (POSIX maximal munch: <<- beats << beats
   <, && beats &). Token types and lengths are byte-identical to the old
   table walk. */

/* '<' family: heredocs, process substitution, dup, read-write, redirect. */
static int	op_left(const char *s, t_tt *t)
{
	if (s[1] == '<' && s[2] == '<')
		return (*t = TT_HERESTRING, 3);
	if (s[1] == '<' && s[2] == '-')
		return (*t = TT_HEREDOC, 3);
	if (s[1] == '<')
		return (*t = TT_HEREDOC, 2);
	if (s[1] == '(')
		return (*t = TT_PROC_SUB_IN, 2);
	if (s[1] == '&')
		return (*t = TT_DUP_IN, 2);
	if (s[1] == '>')
		return (*t = TT_READWRITE, 2);
	return (*t = TT_REDIRECT_LEFT, 1);
}

/* '>' family: append, process substitution, dup, clobber, redirect. */
static int	op_right(const char *s, t_tt *t)
{
	if (s[1] == '>')
		return (*t = TT_APPEND, 2);
	if (s[1] == '(')
		return (*t = TT_PROC_SUB_OUT, 2);
	if (s[1] == '&')
		return (*t = TT_DUP_OUT, 2);
	if (s[1] == '|')
		return (*t = TT_CLOBBER, 2);
	return (*t = TT_REDIRECT_RIGHT, 1);
}

/* Everything else: pipe/or, amp/and, semi/dsemi, parens/arith, and zsh's
   `=(cmd)` process substitution to a temp file. That last one is gated on
   the dialect through the mirrored cell -- in bash a leading `=` is an
   ordinary word character, and `=(x)` there is a word followed by a
   subshell. Returns 0
   for a character that is no operator at all — the caller asserts, since
   is_word_boundary should never have routed such a byte here. */
static int	op_other(const char *s, t_tt *t)
{
	if (s[0] == '|' && s[1] == '|')
		return (*t = TT_OR, 2);
	if (s[0] == '|')
		return (*t = TT_PIPE, 1);
	if (s[0] == '&' && s[1] == '&')
		return (*t = TT_AND, 2);
	if (s[0] == '&')
		return (*t = TT_AMPERSAND, 1);
	if (s[0] == ';' && s[1] == ';')
		return (*t = TT_DSEMI, 2);
	if (s[0] == ';')
		return (*t = TT_SEMICOLON, 1);
	if (s[0] == '=' && s[1] == '(' && glob_zsh())
		return (*t = TT_PROC_SUB_FILE, 2);
	if (s[0] == '(' && s[1] == '(')
		return (*t = TT_ARITH_START, 2);
	if (s[0] == '(')
		return (*t = TT_BRACE_LEFT, 1);
	if (s[0] == ')')
		return (*t = TT_BRACE_RIGHT, 1);
	return (0);
}

/* Emit the operator token that starts at *str. First try the fd-prefixed
   form (e.g. `2>`); if none, dispatch on the leading character. The
   ft_assert guards against reaching a character that is neither a word
   nor a recognised operator — that would be a bug in is_word_boundary. */
void	parse_op(t_deque_tok *tokens, char **str)
{
	char	*start;
	t_tt	type;
	int		len;
	t_token	tmp;

	len = check_fd_redirect(*str, &tmp);
	if (len > 0)
	{
		*str += len;
		push_ltok(tokens, tmp);
		return ;
	}
	start = *str;
	type = TT_END;
	if (*start == '<')
		len = op_left(start, &type);
	else if (*start == '>')
		len = op_right(start, &type);
	else
		len = op_other(start, &type);
	ft_assert(len > 0);
	*str += len;
	tmp = create_token(start, len, type);
	push_ltok(tokens, tmp);
}
