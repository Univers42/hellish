/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redir.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:25:11 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:25:11 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REDIR_H
# define REDIR_H

# include "common.h"

typedef t_vec	t_redir;

typedef struct s_redir
{
	bool	direction_in;
	int		fd;
	char	*fname;
	bool	should_delete;
}	t_redir;

# endif