/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_vcs3.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* A zsh-style boolean style value: what `zstyle ... check-for-changes`
   and HELLISH_VCS_UNTRACKED accept as "on". */
bool	vcs_truthy(const char *v)
{
	return (v && (!ft_strcmp(v, "true") || !ft_strcmp(v, "yes")
			|| !ft_strcmp(v, "1") || !ft_strcmp(v, "on")));
}
