/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore_bash_trapsig.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 00:49:25 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 09:01:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

// Stubs for external variable functions
void	unbind_variable_noref(const char *name)
{
	(void)name;
}

void	bind_variable(const char *name, const char *value, int flags)
{
	(void)name;
	(void)value;
	(void)flags;
}

void	bind_var_to_int(const char *name, int value, int flags)
{
	(void)name;
	(void)value;
	(void)flags;
}

char	*get_string_value(const char *name)
{
	(void)name;
	return (NULL);
}

char	*savestring(const char *s)
{
	if (!s)
		return (NULL);
	char *ret = malloc(strlen(s) + 1);
	if (ret)
		strcpy(ret, s);
	return (ret);
}

void	restore_bash_trapsig(char *oldval)
{
	if (oldval == 0)
		unbind_variable_noref("SH_TRAPSIG");
	else
	{
		bind_variable("SH_TRAPSIG", oldval, 0);
		free(oldval);
	}
}
