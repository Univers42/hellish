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

/* Redirection entry record (stored inside t_redir vector) */
typedef struct s_redir_entry
{
	bool	direction_in;
	int		fd;
	char	*fname;
	bool	should_delete;
}	t_redir_entry;

/* t_redir is a vector container (t_vec) holding t_redir_entry elements */
/* typedef t_vec t_redir;  -- already defined in dsa.h */

#endif