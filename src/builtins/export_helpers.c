/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   export_helpers.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:02:29 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:59:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Split an export argument into its name and optional value parts. If `str`
   contains '=', everything before it is the identifier and everything after
   is the value. Without '=', *val is left NULL (mark existing var exported
   without changing its value). Both *ident and *val are heap-allocated. */
/* True when `a` is an UNRECOGNISED option word for export/unset: it begins
   with '-' and is longer than a lone '-', and either is a "--xxx" long
   option (only the bare "--" end-of-options marker is accepted) or clusters
   an option letter not in `valid`.  A lone '-' and plain NAMEs are operands.
   bash rejects e.g. `export -TEST=1` / `unset -x` with status 2 -- this is
   how we detect that. */
bool	bad_opt_word(const char *a, const char *valid)
{
	int	j;

	if (a[0] != '-' || a[1] == '\0')
		return (false);
	if (a[1] == '-')
		return (a[2] != '\0');
	j = 1;
	while (a[j])
	{
		if (!ft_strchr(valid, a[j]))
			return (true);
		j++;
	}
	return (false);
}

void	parse_export_arg(char *str, char **ident, char **val)
{
	char	*eq;

	eq = ft_strchr(str, '=');
	if (eq)
	{
		*ident = ft_strndup(str, eq - str);
		*val = ft_strdup(eq + 1);
	}
	else
	{
		*ident = ft_strdup(str);
		*val = NULL;
	}
}

/* Validate an identifier per POSIX: starts with a letter or underscore
   (is_var_name_p1), followed by zero or more letters/digits/underscores
   (is_var_name_p2), and nothing else. Returns false for empty strings too. */
bool	ft_is_valid_ident(char *id)
{
	int	i;

	i = 0;
	if (!is_var_name_p1(id[0]))
		return (false);
	while (id[i] && is_var_name_p2(id[i]))
		i++;
	return (!id[i]);
}

/* If *val is enclosed in matching single or double quotes, remove them and
   replace *val with the unquoted copy. Returns the quote character (so the
   caller knows whether to expand the value) or '\0' if no quotes were found.
   The old *val is freed, so this is an in-place replace. */
char	strip_surrounding_quotes(char **val)
{
	size_t	vlen;
	char	*clean;
	char	f;

	if (!val || !*val)
		return (0);
	vlen = ft_strlen(*val);
	if (vlen >= 2)
	{
		f = (*val)[0];
		if ((f == '"' || f == '\'') && (*val)[vlen - 1] == f)
		{
			clean = ft_strndup(*val + 1, vlen - 2);
			xfree(*val);
			*val = clean;
			return (f);
		}
	}
	return (0);
}
