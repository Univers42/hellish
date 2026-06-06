/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:13:58 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 00:13:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

/* Script file failed to open. We distinguish EISDIR (exit 127) from all
   other errno values (EXE_BASE_ERROR + errno) so scripts calling "hellish
   some_dir" get the same non-zero-but-not-1 code as bash. Print the system
   error message first so the user actually knows what went wrong, then free
   everything -- we're done. */
void	handle_file_open_error(t_shell *state, char **argv)
{
	ft_eprintf("%s: %s: %s\n", state->dft_ctx, argv[1], strerror(errno));
	free_all_state(state);
	if (errno == EISDIR)
		exit(127);
	exit(EXE_BASE_ERROR + errno);
}
