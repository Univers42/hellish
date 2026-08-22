/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_files.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* File completion: delegate entirely to readline's built-in filename
   generator.  We just clear the append char (no trailing space after a
   directory completion -- readline appends '/' instead automatically).

   The second entry point here is the same generator with an executable
   filter in front of it, for a command word that contains a slash. */

#include "libft.h"
#include <stdio.h>
#include <readline/readline.h>
#include <readline/tilde.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

/* Hand off to rl_filename_completion_function, which handles tilde
   expansion, hidden files, and directory slash appending for us. */
char	**complete_files(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = '\0';
	return (rl_completion_matches(text, rl_filename_completion_function));
}

/* readline's filename generator with everything unrunnable filtered out.
   Directories stay: you may be typing THROUGH one on the way to a
   program, and readline marks them with a trailing '/' anyway.

   The tilde_expand() is not decoration -- rl_filename_completion_function
   hands back the match with the user's "~/" still on the front, and
   stat("~/bin/foo") does not resolve.  Both strings come from readline's
   allocator, so both are released with libc free(); nothing here may use
   xfree (see rl_dup in completion.c for the whole story). */
static char	*exec_file_gen(const char *text, int state_gen)
{
	char		*name;
	char		*real;
	struct stat	st;

	while (1)
	{
		name = rl_filename_completion_function(text, state_gen);
		state_gen = 1;
		if (!name)
			return (NULL);
		real = tilde_expand(name);
		if (real && stat(real, &st) == 0 && (S_ISDIR(st.st_mode)
				|| access(real, X_OK) == 0))
			return (free(real), name);
		free(real);
		free(name);
	}
}

/* Command completion for a word that holds a '/'.  POSIX does not search
   PATH for such a word -- it names the file directly -- so offering every
   file next to it is wrong in the same way offering documents from a PATH
   directory was: `./<TAB>` should list what can be run, not the folder. */
char	**complete_exec_files(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = '\0';
	return (rl_completion_matches(text, exec_file_gen));
}
