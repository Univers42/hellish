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

typedef struct s_env
{
	bool	exported;
	char	*key;
	char	*value;
}t_env;

// in buffer add the t_env *buff;
typedef t_vec t_envs;



#endif