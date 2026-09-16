/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_cmdpos.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "completion_private.h"
#include "libft.h"
#include <readline/readline.h>

/* Is the word beginning at `start` a COMMAND word?
**
** It is when nothing but blanks precede it, and it is when the last
** non-blank before it ends a command: POSIX XCU 2.9 makes the word after
** ; | & ( { ` and a newline the start of a new command, which is why
** `ls | <TAB>` and `true && <TAB>` complete a command in bash. This used
** to be `start == 0`, so all of those -- and a line that merely began
** with a space -- fell through to filename completion instead.
**
** A RESERVED WORD ends a command too, and that half was still missing:
** after `then`, `do`, `else`, `elif`, `in`, `!` or `time` the next word
** is a command name, so
**
**     for i in 1 2; do ec<TAB>
**
** offered the files in the current directory where bash offers `echo`.
** The words that introduce a command without being one -- `command`,
** `exec`, `sudo`, `env`, `nohup` -- belong to the same set, which is why
** bash completes `sudo ec<TAB>` as a command as well.
**
** The redirection operators are deliberately NOT in the set even though
** readline breaks words on them: the word after > or < is a filename,
** and so is the word after the = of an assignment. */

/* The word [s, s+n) is one that leaves the next word in command position. */
static int	kw_ends_cmd(const char *s, int n)
{
	static const char	*kw[] = {"then", "do", "else", "elif", "in", "!",
		"time", "command", "exec", "sudo", "nohup", "env", "while", "until",
		"if", "for", NULL};
	int					i;

	i = -1;
	while (kw[++i])
	{
		if ((int)ft_strlen(kw[i]) == n && !ft_strncmp(s, kw[i], (size_t)n))
			return (1);
	}
	return (0);
}

int	is_cmd_word(int start)
{
	int	i;
	int	e;

	if (!rl_line_buffer)
		return (start == 0);
	i = start;
	while (i > 0 && (rl_line_buffer[i - 1] == ' '
			|| rl_line_buffer[i - 1] == '\t'))
		i--;
	if (i == 0)
		return (1);
	if (ft_strchr(";|&(){\n`", rl_line_buffer[i - 1]))
		return (1);
	e = i;
	while (i > 0 && !ft_strchr(" \t;|&(){\n`", rl_line_buffer[i - 1]))
		i--;
	return (kw_ends_cmd(rl_line_buffer + i, e - i));
}
