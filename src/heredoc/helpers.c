/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:09 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:09 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

/* Special/positional param names are a single char: digit ($1..$9),
** or $#, $?, $$, $!, $@, $* (not recognised by env_len). */
static int	special_param_len(char c)
{
	if (ft_isdigit((unsigned char)c) || ft_strchr("#?$!@*", c))
		return (1);
	return (0);
}

/* Expand a $ that was not followed by '(' (command substitution already
   handled by the caller).  Braced form ${...} -> expand_braced; plain
   name or special param -> env_expand_n; lone $ (no valid name follows)
   -> emit the $ character literally and leave the byte after it to the
   caller (it used to be pushed here AND by the loop, so `$ 0` came out as
   `  0` -- which is the `line = $ 0` in every autoconf config.status, and
   turned the generated Makefile into 4666 lines of `0`).  *i is advanced
   past whatever was consumed. */
void	expand_dolar(t_shell *state, int *i, t_string *full_file, char *line)
{
	int		len;
	char	*env;

	(*i)++;
	if (line[*i] == '{')
		return (expand_braced(state, i, full_file, line));
	len = env_len(line + *i);
	if (len == 0)
		len = special_param_len(line[*i]);
	if (len)
	{
		if (!full_file->ctx)
			vec_init(full_file);
		env = env_expand_n(state, line + *i, len);
		if (env)
			vec_push_str(full_file, env);
		else
			vec_push_nstr(full_file, "", 0);
	}
	else
		vec_push(full_file, &line[*i - 1]);
	*i += len;
}

/* Handle one character following a backslash inside a non-quoted heredoc
   body.  POSIX: inside a heredoc only `\$`, `\`` and `\\` are special --
   other `\x` pairs keep the backslash.  A `\<newline>` (line continuation)
   is simply dropped (the backslash and the newline disappear).  The
   backquote is checked here rather than in libft's is_escapable: that
   predicate is the generic $/backslash pair, and the backquote only became
   an escape once expand_line started running `...` in bodies. */
void	expand_bs(int *i, t_string *full_file, char *line)
{
	char	tmp;

	if (is_escapable(line[*i]) || line[*i] == '`')
		vec_push(full_file, &line[*i]);
	else if (line[*i] != '\n')
	{
		tmp = '\\';
		vec_push(full_file, &tmp);
		tmp = line[*i];
		vec_push(full_file, &tmp);
	}
	(*i)++;
}

/* Dispatch a '$' or a '`': $(...) / $((...)) and `...` go through the
   expander's substitution entries; a '$' they do not claim falls through to
   plain parameter expansion, and an unclosed backquote stays literal. */
static void	expand_sub_char(t_shell *state, t_string *ff, char *line, int *i)
{
	int	consumed;

	if (line[*i] == '`')
		consumed = expand_backquote_sub(state, line + *i, ff);
	else
		consumed = expand_dollar_sub(state, line + *i,
				(int)ft_strlen(line + *i), ff);
	if (consumed > 0)
		*i += consumed;
	else if (line[*i] == '`')
		vec_push(ff, &line[(*i)++]);
	else
		expand_dolar(state, i, ff, line);
}

/* Expand one heredoc body line into full_file.  A backslash arms a
   one-character escape (bs=true), which is then handled by expand_bs on
   the next iteration.  A '$' kicks off parameter/command substitution and
   a '`' a backquoted one -- autoconf writes `$as_echo ... | $as_tr_cpp`
   into confdefs.h from a heredoc, and a body that keeps the backquotes
   literal turns every later header check into a compile error.  An
   unclosed backquote stays literal.  Any other character is appended
   verbatim.  The result is accumulated in
   full_file (a t_string / vec of bytes) rather than printed immediately so
   write_heredoc can write the whole body in one shot. */
void	expand_line(t_shell *state, t_string *full_file, char *line)
{
	int		i;
	bool	bs;

	i = 0;
	bs = false;
	while (line[i])
	{
		if (bs)
		{
			expand_bs(&i, full_file, line);
			bs = false;
		}
		else if (line[i] == '$' || line[i] == '`')
			expand_sub_char(state, full_file, line, &i);
		else if (line[i] == '\\')
			bs = (i++, true);
		else
			vec_push(full_file, &line[i++]);
	}
}
