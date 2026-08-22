/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   completion.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Tab completion entry point.  We install one function
   (rl_attempted_completion_function) and let readline call it for every
   TAB.  The function dispatches on what the word IS: a '$' word is a
   variable, a word in command position is a command (see is_cmd_word --
   that is not the same thing as column 0), and anything else is left to
   readline's own filename completion by returning NULL. */

#include "completion_private.h"
#include "libft.h"
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <dirent.h>

char	**complete_commands(const char *text, int start, int end);
char	**complete_variables(const char *text, int start, int end);
char	**complete_files(const char *text, int start, int end);
char	**complete_exec_files(const char *text, int start, int end);

/* readline OWNS every string a generator hands back, and releases it with
   libc free(). Our xmalloc family compiles to ft_malloc on a SAFE=0 build,
   so an ft_strdup'd match was a pointer freed by the wrong allocator:
   glibc aborted on the very first TAB with "free(): invalid size" -- or,
   depending on what PATH happened to hold, "double free or corruption" --
   and the line editor never came back (issue #40). It bit every OPT=1
   build, which is the one the README recommends for daily use.

   So these two are the shell's ONE sanctioned use of libc malloc, and the
   ownership handoff is the whole reason. Nothing else here may allocate a
   string that readline will free. */
char	*rl_dup(const char *s)
{
	size_t	n;
	char	*out;

	n = ft_strlen(s);
	out = malloc(n + 1);
	if (!out)
		return (NULL);
	return ((char *)ft_memcpy(out, s, n + 1));
}

/* Same handoff for variable completion: readline inserts what we return
   verbatim, so the '$' has to be part of the returned string rather than
   glued on afterwards by a second allocation. */
char	*rl_dup_dollar(const char *name, size_t len)
{
	char	*out;

	out = malloc(len + 2);
	if (!out)
		return (NULL);
	out[0] = '$';
	ft_memcpy(out + 1, name, len);
	out[len + 1] = '\0';
	return (out);
}

/* Is the word beginning at `start` a COMMAND word?

   It is when nothing but blanks precede it, and it is when the last
   non-blank before it is an operator that ends a command: POSIX XCU 2.9
   makes the word after ; | & ( { ` and a newline the start of a new
   command, which is why `ls | <TAB>` and `true && <TAB>` complete a
   command in bash.  This used to be `start == 0`, so all of those --
   and a line that merely began with a space -- silently fell through to
   filename completion instead.

   The redirection operators are deliberately NOT in the set even though
   readline breaks words on them: the word after > or < is a filename,
   and so is the word after the = of an assignment. */
static int	is_cmd_word(int start)
{
	int	i;

	if (!rl_line_buffer)
		return (start == 0);
	i = start;
	while (i > 0 && (rl_line_buffer[i - 1] == ' '
			|| rl_line_buffer[i - 1] == '\t'))
		i--;
	if (i == 0)
		return (1);
	return (ft_strchr(";|&(){\n`", rl_line_buffer[i - 1]) != NULL);
}

/* Completion dispatcher registered with readline.  A '$' word is a
   variable wherever it appears -- this test comes FIRST because the old
   start==0 branch shadowed it, so `$HOM<TAB>` at the start of a line was
   sent to the PATH scan and completed to nothing.  A command word holding
   a '/' is not searched on PATH (it IS the path), so it goes to the
   executable-filtered file completer.  Returning NULL leaves readline's
   own filename completion in charge, which is what an argument wants.

   rl_attempted_completion_over says "I answered this one, do not fall
   back", and it is the other half of the bug.  Without it, a NO-MATCH
   answer from either completer is indistinguishable from "not mine", and
   readline quietly retries the word as a plain filename -- so a command
   word still ended up offering the documents sitting in the current
   directory, which is the same wrong list arriving by a second route.
   bash sets this flag for exactly this reason; with it, an unknown
   command name just dings, as it should. */
static char	**cmd_completion(const char *text, int start, int end)
{
	if (text[0] == '$')
		return (rl_attempted_completion_over = 1,
			complete_variables(text, start, end));
	if (!is_cmd_word(start))
		return (NULL);
	rl_attempted_completion_over = 1;
	if (ft_strchr(text, '/'))
		return (complete_exec_files(text, start, end));
	return (complete_commands(text, start, end));
}

/* Register our completion function and set the default append char to
   space.  Must be called once after readline is initialised; subsequent
   calls are harmless (just overwrites the same pointers).

   The word-break set is readline's default with '$' removed, which is
   also why bash keeps '$' out of its own. With the stock set readline
   split "$HOM" into a word starting at the 'H', so the dispatcher's
   text[0] == '$' test below could never be true and complete_variables
   was unreachable: $VAR completion had simply never worked. */
void	setup_completion(void)
{
	static char	brk[] = " \t\n\"\\'`@><=;|&{(";

	rl_attempted_completion_function = cmd_completion;
	rl_completion_append_character = ' ';
	rl_completer_word_break_characters = brk;
}
