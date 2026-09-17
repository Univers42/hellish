/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_quote.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 22:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 22:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "completion_private.h"
#include "libft.h"
#include <readline/readline.h>
#include <stdlib.h>

/* Quoting for completed filenames.
**
**     touch 'my file.txt' ; cat my<TAB>
**       bash: cat my\ file.txt      here: cat my file.txt
**
** The second one is not the file. It is two words, and the command runs
** against neither of them -- the completion produced a line that cannot
** work, which is worse than producing nothing.
**
** readline does none of this by itself: it inserts what the generator
** returns, verbatim. bash installs four hooks to make a completed word
** survive being read back as shell input, and this file is those four.
** Nothing in the tree set any of them, which is also why a word typed
** inside quotes (`cat "my fi<TAB>`) broke at the space AND at the quote.
**
** Allocation here is the one sanctioned libc malloc, for the reason
** rl_dup gives in completion.c: readline owns and frees what it is
** handed. */

/* Is line[idx] inside quotes, or backslash-escaped?
**
** readline asks this about each word-break character it finds, and skips
** breaking where the answer is yes -- which is what makes `my\ file` one
** word rather than two. */
int	comp_char_is_quoted(char *line, int idx)
{
	int	i;
	int	sq;
	int	dq;

	i = 0;
	sq = 0;
	dq = 0;
	while (i < idx && line[i])
	{
		if (!sq && line[i] == '\\' && line[i + 1])
		{
			if (i + 1 >= idx)
				return (1);
			i += 2;
			continue ;
		}
		if (!dq && line[i] == '\'')
			sq = !sq;
		else if (!sq && line[i] == '"')
			dq = !dq;
		i++;
	}
	return (sq || dq);
}

/* Strip the quoting off the word before it is matched against real names:
   the user typed `my\ fi`, the directory holds `my file.txt`. */
char	*comp_dequote_filename(char *text, int qc)
{
	char	*out;
	int		i;
	int		j;

	out = malloc(ft_strlen(text) + 1);
	if (!out)
		return (NULL);
	i = 0;
	j = 0;
	while (text[i])
	{
		if (text[i] == '\\' && qc != '\'' && text[i + 1])
			i++;
		else if (qc == 0 && (text[i] == '"' || text[i] == '\''))
		{
			i++;
			continue ;
		}
		out[j++] = text[i++];
	}
	return (out[j] = '\0', out);
}

/* The head of the word that the user wrote in order to have it EXPANDED:
   a leading `~` or `$NAME`/`${NAME}`. Quoting it is what two field reports
   were: `ls ~/.con<TAB>` inserted `ls \~/.config/`, and once paths through
   a variable completed, `ls $HOME/.con<TAB>` inserted `ls \$HOME/.config/`.
   Both name something that does not exist -- a backslashed `~` is a
   directory called "~", a backslashed `$` is a literal dollar. These
   characters are in the quote set because in the MIDDLE of a filename they
   are ordinary and need escaping; at the head they are the whole point,
   and bash leaves them alone there too. */
static size_t	comp_literal_prefix(const char *s)
{
	size_t	brace;
	size_t	n;

	if (s[0] == '~')
		return (1);
	if (s[0] != '$')
		return (0);
	brace = 0;
	if (s[1] == '{')
		brace = 1;
	n = comp_dollar_len(s + 1 + brace);
	if (n == 0 || (brace && s[1 + brace + n] != '}'))
		return (0);
	return (1 + brace + n + brace);
}

/* Put the quoting back on the match being inserted. Inside an open quote
   readline supplies the closing one and the shell reads the span
   literally, so only the bare case needs backslashes. */
char	*comp_quote_filename(char *text, int mtype, char *qp)
{
	char	*out;
	size_t	i;
	size_t	j;

	(void)mtype;
	out = malloc(ft_strlen(text) * 2 + 1);
	if (!out)
		return (NULL);
	i = comp_literal_prefix(text);
	ft_memcpy(out, text, i);
	j = i;
	while (text[i])
	{
		if ((!qp || !*qp) && rl_filename_quote_characters
			&& ft_strchr((char *)rl_filename_quote_characters, text[i]))
			out[j++] = '\\';
		out[j++] = text[i++];
	}
	return (out[j] = '\0', out);
}

/* Install the four. The quote-character set is bash's, so a name that
   completes here is a name that pastes into bash unchanged. */
void	setup_quoting(void)
{
	static char	qchars[] = " \t\n\\\"'@<>=;|&()#$`?*[]!:{}~";

	rl_completer_quote_characters = "'\"";
	rl_filename_quote_characters = qchars;
	rl_char_is_quoted_p = comp_char_is_quoted;
	rl_filename_quoting_function = comp_quote_filename;
	rl_filename_dequoting_function = comp_dequote_filename;
}
