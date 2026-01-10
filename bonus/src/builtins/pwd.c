/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pwd.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:42 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:42 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins.h"

int	builtin_pwd(t_state *state, t_vec argv)
{
	(void)argv;
	if (state->cwd.ctx == NULL)
		ft_eprintf("pwd: error retrieving current directory:"
			" getcwd: cannot access parent directories:"
			" No such file or directory\n");
	else
		ft_printf("%s\n", (char *)state->cwd.ctx);
	return (0);
}
