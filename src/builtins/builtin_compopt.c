/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compopt.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "libft.h"

/* `compopt` -- the option half of programmable completion, and the piece
** whose absence made bash-completion look broken rather than missing.
**
** bash-completion does not put `-o filenames` on its registrations. It
** calls `compopt -o filenames` from INSIDE the completion function, once
** it knows the candidates are paths -- twenty-odd sites, `_filedir` and
** `_comp_complete_longopt` among them. hellish had no such builtin, so
** every one of those calls was a command-not-found, the option never
** arrived, and readline appended no `/` to a directory:
**
**     ls /us<TAB>     bash: ls /usr/        hellish: ls /usr
**
** which is exactly the "completion does not work in the first shell"
** report from the born2root VM -- the login shell sources /etc/profile,
** picks up bash-completion's specs, and every path completion loses its
** slash; a nested shell loads no specs and falls back to readline's own
** filename completion, which was always right.
*/

/* bash's nine completion options, in the order bash prints them; NULL
   past the last, so a caller walks i upwards until it stops. */
char	*co_name_at(int i)
{
	static const char *const	n[] = {"bashdefault", "default", "dirnames",
		"filenames", "fullquote", "noquote", "nosort", "nospace",
		"plusdirs", NULL};

	if (i < 0 || i >= 9)
		return (NULL);
	return ((char *)n[i]);
}

/* Does N belong in the rebuilt list? NAME is the one being changed. */
static bool	co_wanted(const char *opts, const char *n, const char *name,
		bool on)
{
	if (name && !ft_strcmp(n, name))
		return (on);
	return (pc_opt_has(opts, n));
}

/* Add or remove NAME in the space-joined list *OPTS, in place. The list is
   rebuilt from the name table rather than patched: it is nine short words
   at most, and rebuilding is the version that cannot leave a stray
   separator behind or reorder what `complete -p` prints. */
void	co_set(char **opts, const char *name, bool on)
{
	char		*n;
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = -1;
	while (++i < 9)
	{
		n = co_name_at(i);
		if (!co_wanted(*opts, n, name, on))
			continue ;
		if (out.len)
			vec_push_char(&out, ' ');
		vec_push_str(&out, n);
	}
	vec_push_char(&out, '\0');
	xfree(*opts);
	*opts = (char *)out.ctx;
}

/* Apply every -o/+o in the command line to one option list. bash applies
   the same set of edits to each name it was given, so the walk is over
   argv and the target is the caller's. */
void	co_edit(t_vec argv, char **opts)
{
	size_t	i;
	char	*w;

	i = 1;
	while (i + 1 < argv.len)
	{
		w = ((char **)argv.ctx)[i];
		if (!ft_strcmp(w, "-o"))
			co_set(opts, ((char **)argv.ctx)[i + 1], true);
		else if (!ft_strcmp(w, "+o"))
			co_set(opts, ((char **)argv.ctx)[i + 1], false);
		else
		{
			i++;
			continue ;
		}
		i += 2;
	}
}

/* Was any -o/+o given? `compopt name` with none is a PRINT, not a no-op
   edit -- bash echoes the spec's whole option state back. */
bool	co_has_edit(t_vec argv)
{
	size_t	i;
	char	*w;

	i = 1;
	while (i + 1 < argv.len)
	{
		w = ((char **)argv.ctx)[i++];
		if (!ft_strcmp(w, "-o") || !ft_strcmp(w, "+o"))
			return (true);
	}
	return (false);
}
