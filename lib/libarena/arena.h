/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   arena.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/06 15:30:53 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/06 20:36:12 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ARENA_H
# define ARENA_H

# include <stddef.h>
# include <stdlib.h>

/*
** Minimal constant and types. No macro functions; API is function-based.
*/
# define ARENA_MINSIZE	504

typedef struct s_stack_mark
{
	struct s_stack_block	*stackp;
	char					*stacknxt;
	size_t					stacknleft;
}	t_stack_mark;

typedef struct s_stack_block
{
	struct s_stack_block	*prev;
	char					space[ARENA_MINSIZE];
}	t_stack_block;

typedef struct s_garena
{
	t_stack_block	stack_base;
	t_stack_block	*stackp;
	char			*stack_next;
	size_t			stack_nleft;
	char			*sstrend;
}	t_garena;

typedef struct s_dynall
{
	size_t			aligned;
	size_t			block_size;
	size_t			len;
	t_stack_block	*p;
}	t_dynall;

/* Context */
t_garena	*arena_ctx(void);
void		arena_cleanup(void);

/* Core allocator (generic, not only strings) */
void		*st_alloc(size_t nbytes);
void		st_unalloc(void *p);

/* Growing the active stack block/string */
char		*grow_stack_block(size_t min);
void		*grow_stack_str(void);
char		*grow_stack_to(size_t len);
char		*make_str_space(size_t newlen, char *p);
void		grab_stack_block(size_t len);

/* Stack mark helpers */
void		push_stack_mark(t_stack_mark *mark, size_t len);
void		set_stack_mark(t_stack_mark *mark);
void		pop_stack_mark(t_stack_mark *mark);

/* String helpers on top of arena */
char		*stack_block(void);
size_t		stack_block_size(void);
char		*start_stack_str(void);
char		*stack_str_nul(char *p);

char		*st_nputs(const char *s, size_t n, char *p);
char		*st_puts(const char *s, char *p);
char		*st_putc(int c, char *p);
char		*st_unputc(char *p);
int			st_topc(char *p);
char		*st_adjust(ptrdiff_t amount, char *p);
void		ungrab_stack_str(void *s);
void		grab_stack_str(char *p);

/* Generic helpers */
size_t		arena_align(size_t nbytes);
size_t		min_size(size_t nbytes);
void		out_of_space(void);
void		*check_null(void *p);
char		*str_save(const char *s);
void		*arena_malloc(size_t nbytes);
void		*arena_realloc(void *p, size_t nbytes);
void		arena_free(void *p);
size_t		compute_new_len(size_t min, t_garena *g);

#endif