/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alcacere <alcacere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 19:43:36 by alcacere          #+#    #+#             */
/*   Updated: 2025/11/04 19:57:06 by alcacere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VAR_H
# define VAR_H

/**
 * Fumnction to be called when 
 */
typedef struct s_var
{
	int			flags;					// flags are defined above
	const char	*text;					// name=value
	void		(*func)(const char *);	//called when the var gets set/unset
	struct var	*next;					// next entry in hash list
}	t_var;

int	varcmp(const char *p, const char *q);

#endif