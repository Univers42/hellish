/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_command2.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/09 23:30:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Alias expansion no longer happens here: it moved to the input scanner
   (src/alias/alias_scan.c), which runs before the lexer so quoting,
   keywords in alias bodies, recursion and the trailing-blank rule all
   behave like bash.  Exec-time argv splicing could honor none of those. */

/* After glob/IFS expansion some argv slots can be NULL or a low pointer
   (sentinel values from the slab allocator).  execve(2) would crash or
   behave oddly if it saw those, so we replace any such slot with a fresh
   empty string.  The uintptr_t < 4096 guard catches slab sentinels that
   are not literally NULL but still invalid as C strings. */
void	replace_null_argv_with_empty(t_executable_cmd *cmd)
{
	size_t	i;
	char	*p;

	i = 0;
	while (i < cmd->argv.len)
	{
		p = ((char **)cmd->argv.ctx)[i];
		if (p == NULL || (uintptr_t)p < 4096)
			((char **)cmd->argv.ctx)[i] = ft_strdup("");
		i++;
	}
}

/* Restore the three standard fds from a bak[3] produced by dup().  Used
   by callers that saved before redirecting (exec_builtin, func_call).
   Must be paired with take_backup_fds and called even on error paths to
   avoid leaking the dup'd fds. */
void	restore_fds(int *bak)
{
	dup2(bak[0], 0);
	dup2(bak[1], 1);
	dup2(bak[2], 2);
	close(bak[0]);
	close(bak[1]);
	close(bak[2]);
}
