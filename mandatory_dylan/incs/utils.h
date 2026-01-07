/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 17:29:02 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 17:29:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_H
# define UTILS_H

/* Included via common.h (do not include common.h here to avoid circular includes) */

# include <stdint.h>
# include <stdio.h>
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>
# include <unistd.h>
# include <stdbool.h>
# include <sys/types.h>

/* Map commonly used ft_* names to libc equivalents where practical so this
   header can be included early without depending on other project headers. */
# ifndef FT_COMPAT_MACROS
#  define FT_COMPAT_MACROS
#  define ft_strchr   strchr
#  define ft_toupper  toupper
#  define ft_strlen   strlen
#  define ft_strcmp   strcmp
#  define ft_isalpha  isalpha
#  define ft_isalnum  isalnum
#  define ft_strncmp  strncmp
#  define ft_strdup   strdup
#  define ft_strndup  strndup
# endif

typedef struct s_prng_state
{
	uint32_t	state_array[624];
	int			state_index;
}	t_prng_state;

bool		is_space(int c);
bool		is_special_char(int c);
void		prng_init_state(t_prng_state *state, uint32_t seed);
uint32_t	random_uint32(t_prng_state *state);
char		*getpid_hack(void);
int			mini_atoi_base(char **str, int base, int len);
void		parse_numeric_escape(char **str);
int			backslash_writer(char *s);
int			e_parser(char *s);
bool		is_var_name_p1(char c);
bool		is_var_name_p2(char c);
bool		is_valid_ident(char *id);
int			write_to_file(char *str, int fd);
void		ft_assert(int cond);
ssize_t		ft_abs(ssize_t n);
ssize_t		ft_smin(ssize_t a, ssize_t b);
size_t		ft_min(size_t a, size_t b);
size_t		ft_max(size_t a, size_t b);
ssize_t		ft_smax(ssize_t a, ssize_t b);
ssize_t		ft_smod(ssize_t a, ssize_t b);
int			ft_strnlen(char *s, int n);
void		ft_fdputmem(int fd, char *s, int n);
void		ft_putmem(char *s, int n);
void		ft_eputmem(char *s, int n);
bool		str_slice_eq_str(char *s, size_t len, char *s2);

# endif