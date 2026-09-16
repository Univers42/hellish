/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp_opts.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "completion_private.h"
#include "progcomp_private.h"
#include <readline/readline.h>

/* `complete -o ...`, finally acted on.
**
** The spec's -o list was recorded and never read -- deliberately at first,
** because a plugin passing `-o nospace` should register and lose the
** nicety rather than fail outright. But two of those names are not
** niceties. `default` and `bashdefault` say "when I produce nothing, fall
** back to the ordinary completion", and git-completion registers both:
**
**     complete -o bashdefault -o default -o nospace -F __git_wrap__git_main git
**
** Without them, progcomp_try claimed the word the moment a spec existed
** and answered with an empty list, so `git add <TAB>` on a path offered
** nothing where bash offers files. */

/* One whitespace-separated word present in the spec's -o list? */
bool	pc_opt_has(const char *opts, const char *name)
{
	size_t	n;

	if (!opts)
		return (false);
	n = ft_strlen(name);
	while (*opts)
	{
		while (*opts == ' ' || *opts == '\t')
			opts++;
		if (!ft_strncmp(opts, name, n)
			&& (opts[n] == '\0' || opts[n] == ' ' || opts[n] == '\t'))
			return (true);
		while (*opts && *opts != ' ' && *opts != '\t')
			opts++;
	}
	return (false);
}

/* Apply the display-affecting options before the matches are generated.
   The append character is reset unconditionally, because it is a sticky
   global: the file completers set it to '\0' and progcomp never set it
   back, so whether `git ch<TAB>` gained a trailing space depended on
   whether the PREVIOUS completion on that line had been a filename. */
void	pc_opt_apply(const char *opts)
{
	rl_completion_append_character = ' ';
	rl_completion_suppress_append = 0;
	if (pc_opt_has(opts, "nospace"))
		rl_completion_suppress_append = 1;
	if (pc_opt_has(opts, "filenames"))
		rl_filename_completion_desired = 1;
}

/* Does this spec want the ordinary completion when it finds nothing? */
bool	pc_opt_fallback(const char *opts)
{
	return (pc_opt_has(opts, "default") || pc_opt_has(opts, "bashdefault"));
}
