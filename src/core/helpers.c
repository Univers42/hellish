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

/* Helper for error handling in init_file */
void	handle_file_open_error(t_shell *state, char **argv)
{
	ft_eprintf("%s: %s: %s\n", state->dft_ctx, argv[1], strerror(errno));
	free_all_state(state);
	if (errno == EISDIR)
		exit(127);
	exit(EXE_BASE_ERROR + errno);
}
