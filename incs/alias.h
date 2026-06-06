/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/11 20:18:34 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:05:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Type aliases that give semantic names to the generic t_vec.
   t_vec is a growable array of any type; the aliases here document what
   a given vec's `ctx` actually points to.  Using distinct names catches
   accidental mix-ups at the call site (passing a t_vec_env where a
   t_string is expected will at least cause a type mismatch warning). */

#ifndef ALIAS_H

# define ALIAS_H

# include "libft.h"

typedef t_vec	t_vec_glob; /* ctx => t_glob* (glob match results) */
typedef t_vec	t_string; /* ctx => char* (byte string / string vec) */
typedef t_vec	t_vec_exe_res; /* ctx => t_execution_state* */
typedef t_vec	t_vec_redir; /* ctx => t_redir* (redirect records) */
typedef t_vec	t_vec_nd; /* ctx => t_ast_node* (AST node children) */
typedef t_vec	t_vec_env; /* ctx => t_env* (environment entries) */

# ifndef T_VEC_INT_DEFINED
#  define T_VEC_INT_DEFINED

typedef t_vec	t_vec_int; /* ctx => int* (redirect index list) */
# endif

#endif
