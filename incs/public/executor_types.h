/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor_types.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:35 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:05:38 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTOR_TYPES_H
# define EXECUTOR_TYPES_H

# include <stdbool.h>

typedef enum s_res_t
{
	RES_OK,
	RES_FatalError,
	RES_MoreInput,
	RES_Init,
}	t_result_type;

typedef struct s_execution_state
{
	int		status;
	int		pid;
	bool	c_c;
}	t_execution_state;

#endif
