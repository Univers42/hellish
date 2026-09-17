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
#include "progcomp_private.h"
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
/* complete_cmdpos.c -- "is this word a command name?", which is not the
   same question as "is it at column 0". */
int		is_cmd_word(int start);

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
   command name just dings, as it should.

   An ARGUMENT word now asks progcomp_try whether the command it belongs to
   has a `complete` spec (#72 phase 4).  It answers NULL and leaves the flag
   alone when there is none, so every command without a registration keeps
   the filename completion it has always had -- which is the case that would
   have been broken silently, and everywhere, by a dispatcher that claimed
   arguments unconditionally. */
static char	**cmd_completion(const char *text, int start, int end)
{
	char	**spec;

	rl_completion_append_character = ' ';
	rl_completion_suppress_append = 0;
	if (text[0] == '$' && !ft_strchr(text, '/'))
		return (rl_attempted_completion_over = 1,
			complete_variables(text, start, end));
	if (!is_cmd_word(start))
	{
		spec = progcomp_try(text, start, end);
		if (rl_attempted_completion_over)
			return (spec);
		return (NULL);
	}
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
   was unreachable: $VAR completion had simply never worked.

   Backslash is out of it too, and bash's set has never had it. A
   backslash in a word is an ESCAPE, not a boundary: with it in the set
   readline cut `cat my\ fi<TAB>` at the backslash and tried to complete
   ` fi`, so continuing a name whose space had already been escaped could
   only ring the bell. The escape is now understood by the quoting hooks
   in complete_quote.c instead -- which is where it belongs, since they
   are what puts the backslash there in the first place. */
void	setup_completion(void)
{
	static char	brk[] = " \t\n\"'`@><=;|&{(";

	rl_attempted_completion_function = cmd_completion;
	rl_completion_append_character = ' ';
	rl_completer_word_break_characters = brk;
	setup_quoting();
	setup_dollar_hooks();
}
