/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vector.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 01:44:02 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 01:44:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VECTOR_H
# define VECTOR_H

# include "common.h"
# include "sys.h"

typedef struct s_vec
{
	size_t	cap;
	size_t	len;
	size_t	elem_size;
	void	*ctx;
}	t_vec;

bool	vec_init(t_vec *ret);
bool	vec_ensure_space(t_vec *v);
bool	vec_ensure_space_n(t_vec *v, size_t n);
bool	vec_push(t_vec *v, const void *el);
bool	vec_push_nstr(t_vec *v, const char *str, size_t n);
bool	vec_push_str(t_vec *v, const char *str);
bool	vec_push_byte(t_vec *v, unsigned char byte);
void	*vec_pop(t_vec *v);
void	*vec_idx(t_vec *v, size_t idx);
void	*vec_ctx_idx(t_vec *v, size_t idx);
void	vec_append_fd(int fd, t_vec *ret);
void	vec_destroy(t_vec *v, void (*free_elem)(void *));
bool	vec_ends_with_str(t_vec *v, const char *str);
void	vec_clear(t_vec *v);

static inline bool	vec_str_ends_with_str(t_string *s, char *s2)
{
	size_t	len_s2;

	len_s2 = ft_strlen(s2);
	if (s->len < len_s2)
		return (false);
	if (ft_strcmp(((char *)s->ctx) + s->len - len_s2, s2) == 0)
		return (true);
	return (false);
}


#endif