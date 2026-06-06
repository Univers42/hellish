/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   find_cmd_path2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 15:10:20 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:53:46 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "cmd_hash.h"

/* A matching file was found in PATH but it lacks execute permission.  Set
   errno=EACCES so err_1_errno prints the right strerror, free the shell
   state (we are inside a forked child), and return EXE_PERM_DENIED (126)
   to propagate the correct exit code up through find_exe_path_wrapper. */
int	handle_perm_denied(t_shell *state, char *cmd_name)
{
	errno = EACCES;
	err_1_errno(state, cmd_name);
	free_all_state(state);
	return (EXE_PERM_DENIED);
}
