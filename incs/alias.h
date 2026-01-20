/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/11 20:18:34 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/21 00:22:49 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ALIAS_H

# define ALIAS_H

# include "libft.h"

typedef t_vec	t_vec_glob;		// ctx => t_glob
typedef t_vec	t_string;		//  ctx => char **
typedef t_vec	t_vec_exe_res;	// ctx => t_exe_res
typedef t_vec	t_vec_redir;	// ctx => t_redir
typedef t_vec	t_vec_nd;		//	ctx => t_ast_node
typedef t_vec	t_vec_env;		// ctx => t_vec_env

# ifndef T_VEC_INT_DEFINED
#  define T_VEC_INT_DEFINED

typedef t_vec	t_vec_int;		// ctx => int *
# endif

#endif
