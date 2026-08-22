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
   TAB.  The function dispatches: position 0 = command, '$' prefix =
   variable, anything else = file.  NULL return falls back to readline's
   default filename completion (which we want for non-dollar arguments). */

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

/* Completion dispatcher registered with readline.  start==0 means we
   are at the command position (no preceding words yet).  text[0]=='$'
   triggers variable completion regardless of position. */
static char	**cmd_completion(const char *text, int start, int end)
{
	(void)end;
	if (start == 0)
		return (complete_commands(text, start, end));
	if (text[0] == '$')
		return (complete_variables(text, start, end));
	return (NULL);
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
