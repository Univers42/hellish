/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   echo_flags.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:17 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* echo's option parsing is surprisingly subtle. An argument is a flag word
   only if it starts with '-' and every subsequent character is one of n/e/E.
   The moment we hit a character that is not in that set we stop scanning for
   flags and treat that argument as the first thing to print — so `echo -n -z`
   prints " -z" rather than silently ignoring -z. */

/* True if `c` is one of the three meaningful flag letters for echo. */
static int	is_flag_char(char c)
{
	return (ft_strchr("nEe", c) != NULL);
}

static void	apply_flag_char(char c, int *tn, int *te)
{
	if (c == 'n')
		*tn = 1;
	else if (c == 'e')
		*te = 1;
	else if (c == 'E')
		*te = 0;
}

/* Try to interpret one argument as a flag token. We work on local copies of
   the n and e flags so a partially-valid token (e.g. "-ne\x00") does not
   corrupt the real flags — only commit when the whole token is valid.
   Returns 1 if it was a valid flag word, 0 if the caller should stop. */
static int	process_flag_token(char *token, int *tn, int *te)
{
	size_t	j;
	int		local_tn;
	int		local_te;
	int		valid;

	local_tn = *tn;
	local_te = *te;
	j = 1;
	valid = 1;
	while (token[j])
	{
		if (!is_flag_char(token[j]))
		{
			valid = 0;
			break ;
		}
		apply_flag_char(token[j], &local_tn, &local_te);
		j++;
	}
	if (!valid)
		return (0);
	*tn = local_tn;
	*te = local_te;
	return (1);
}

/* Scan argv[1..] for flag words. Returns the index of the first non-flag
   argument (which is also the first thing that should be printed). */
int	parse_flags(t_vec argv, int *n, int *e)
{
	size_t	i;

	i = 0;
	while (++i < argv.len && ((char **)argv.ctx)[i][0] == '-' &&
		((char **)argv.ctx)[i][1])
	{
		if (!process_flag_token(((char **)argv.ctx)[i], n, e))
			break ;
	}
	return (i);
}

/* Append argv[i..end] to `out` with a space between each pair. If `e` is
   set, each argument goes through e_parser which handles \n, \t, \c etc. —
   a \c inside any argument causes e_parser to return 1, which stops the
   gathering and signals the caller to suppress the newline too. Building
   into one buffer lets builtin_echo emit a single write(2) instead of one
   per argument/separator (a syscall storm in echo-heavy loops). */
int	print_args(t_vec *out, int e, t_vec argv, size_t i)
{
	while (i < argv.len)
	{
		if (e)
		{
			if (e_parser(out, ((char **)argv.ctx)[i], false))
				return (1);
		}
		else
			vec_push_str(out, ((char **)argv.ctx)[i]);
		if (++i < argv.len)
			vec_push_char(out, ' ');
	}
	return (0);
}
