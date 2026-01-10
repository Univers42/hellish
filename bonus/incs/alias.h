/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:04:01 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:04:01 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ALIAS_H
# define ALIAS_H

# include "infrastructure.h"

typedef t_vec	t_vec_glob;		// ctx => t_glob
typedef t_vec	t_string;		//  ctx => char **
typedef t_vec	t_vec_exe_res;	// ctx => t_exe_res
typedef t_vec	t_vec_redir;	// ctx => t_redir
typedef t_vec	t_vec_nd;		//	ctx => t_ast_node
typedef t_vec	t_vec_env;		// ctx => t_vec_env

# endif