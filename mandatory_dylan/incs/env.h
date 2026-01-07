/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 00:51:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 00:51:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ENV_H
# define ENV_H

# include "common.h"

/* Environment entry record (stored inside t_env vector) */
typedef struct s_env_entry
{
	bool	exported;
	char	*key;
	char	*value;
}	t_env_entry;


#endif