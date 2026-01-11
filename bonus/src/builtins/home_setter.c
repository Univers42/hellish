/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   home_setter.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:46 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins.h"
#include <stdlib.h>

int	cd_home(int *e, t_shell *state)
{
	char	*home;

	home = env_expand(state, "HOME");
	if (home == NULL)
	{
		ft_eprintf("%s: cd: HOME not set\n", state->context);
		return (1);
	}
	*e = chdir(home);
	return (0);
}
