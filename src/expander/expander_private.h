/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_private.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:58:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 17:58:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXPANDER_PRIVATE_H
# define EXPANDER_PRIVATE_H

# include "libft.h"
# include "shell.h"
# include <stdio.h>
# include "env.h"
# include "expander.h"
# include <fcntl.h>
# include <stdbool.h>
# include "ft_glob.h"
# include "helpers.h"
# include <sys/wait.h>
# include <stdlib.h>
# include <string.h>
# include "arith.h"
# include <sys/wait.h>
# include <errno.h>
# include <unistd.h>
# include <sys/wait.h>
# include <signal.h>

typedef struct s_arith_expand_ctx
{
	t_shell		*state;
	const char	*s;
	int			len;
	int			*i;
	t_string	*out;
}	t_arith_expand_ctx;

typedef struct s_seq
{
	long	a;
	long	b;
	long	step;
	int		width;
	bool	alpha;
}	t_seq;

typedef struct s_expand_glob_ctx
{
	t_shell		*state;
	t_ast_node	*node;
	t_vec		*args;
	bool		keep_as_one;
	bool		no_glob;
}	t_expand_glob_ctx;

typedef struct s_trim_ctx
{
	const char	*name;
	int			name_len;
	const char	*op;
	int			slen;
}	t_trim_ctx;

typedef struct s_pe_op
{
	const char	*name;
	int			name_len;
	char		opc;
	bool		colon;
	const char	*word;
	int			wlen;
}	t_pe_op;

typedef struct s_expand_ctx
{
	const char	*s;
	int			slen;
	t_string	*outbuf;
	int			*consumed;
}	t_expand_ctx;

typedef struct s_word_token_ctx
{
	t_shell		*state;
	t_token		*tok;
	t_string	*outbuf;
	int			total_len;
	int			pos;
	bool		changed;
}	t_word_token_ctx;

typedef struct s_env_tok_pos
{
	size_t	idx;
	bool	split_ctx;
}	t_env_tok_pos;

t_string	word_to_string(t_ast_node node);
t_string	word_to_hrdoc_string(t_ast_node node);
t_env		assignment_to_env(t_shell *state, t_ast_node *node);
t_env		assignment_to_env(t_shell *state, t_ast_node *node);
void		assignment_word_to_word(t_ast_node *node);
char		*create_procsub_input(t_shell *state, const char *cmd);
char		*create_procsub_output(t_shell *state, const char *cmd);
char		*expand_proc_sub(t_shell *state, t_ast_node *node);
void		procsub_close_fds_parent(t_shell *state);
void		cleanup_proc_subs(t_shell *state);
bool		create_redir_4(t_tt tt, char *fname, t_redir *ret, int src_fd);
int			parse_src_fd(t_tt tt, t_token op_tok);
int			try_create_redir(t_shell *state, t_ast_node *curr,
				t_tt tt, int src_fd);
char		*expand_param_word(t_shell *state, const char *word, int wlen);
char		*pf_get_var_value(t_shell *state, const char *name, int len);
char		*expand_strlen(t_shell *state, const char *s, int slen);
char		*default_or_alt(t_shell *state, char *val, t_pe_op o);
char		*err_or_assign(t_shell *state, char *val, t_pe_op o);
char		*expand_param_op(t_shell *state, t_pe_op o);
bool		find_param_op(const char *s, int slen, t_pe_op *o);
bool		pat_match_pub(const char *p, const char *s);
char		*trim_suffix_shortest(const char *val, const char *pattern);
char		*trim_suffix_longest(const char *val, const char *pattern);
char		*trim_prefix_shortest(const char *val, const char *pattern);
char		*trim_prefix_longest(const char *val, const char *pattern);
char		*expand_trim(t_shell *state, t_trim_ctx ctx);
bool		parse_seq(const char *body, t_seq *q);
void		run_seq(t_seq q, t_vec *out);
char		*fmt_num(long v, int width);
char		*expand_arith_vars(t_shell *state, const char *s, int len);
void		print_redir_err(t_shell *state,
				t_token_old full_token, char *expanded_name);
int			redirect_from_ast_redir(t_shell *state,
				t_ast_node *curr,
				int *redir_idx);
bool		is_export(t_ast_node word);
int			expand_simple_cmd_assignment(t_shell *state,
				t_expander_simple_cmd *exp, t_executable_cmd *ret);
int			expand_simple_cmd_redir(t_shell *state,
				t_expander_simple_cmd *exp, t_vec_int *redirects);
int			expand_simple_cmd_word(t_shell *state,
				t_expander_simple_cmd *exp, t_executable_cmd *ret);
int			expand_simple_command(t_shell *state, t_ast_node *node,
				t_executable_cmd *ret, t_vec_int *redirects);
void		expand_token(t_shell *state, t_token *curr_tt, bool split_ctx);
void		expand_env_vars(t_shell *state, t_ast_node *node, bool split_ctx);
char		*join_positionals(t_shell *state);
t_ast_node	new_env_node(char *new_start);
void		split_envvar(t_shell *state, t_token *curr_t,
				t_ast_node *curr_node, t_vec_nd *ret);
void		emit_positional_at(t_shell *state, t_ast_node *curr_node,
				t_vec_nd *ret);
bool		ifs_has_nonws(const char *ifs);
bool		try_brace_expand(t_shell *state, t_ast_node *node, t_vec *args);
bool		word_is_plain_literal(t_ast_node *node);
bool		name_is_plain(const char *s, int len);
bool		needs_split_or_glob(const char *v);
t_token		*lone_nonempty_token(t_ast_node *node);
char		*try_simple_envvar(t_shell *state, t_ast_node *node);
bool		is_plain_literal_text(const char *s, int len);
char		*try_simple_concat(t_shell *state, t_ast_node *node);
char		*try_pure_arith(t_shell *state, t_ast_node *node);
char		**ifs_split_posix(const char *s, const char *ifs);
t_vec_nd	split_words(t_shell *state, t_ast_node *node);
t_ast_node	new_env_node(char *new_start);
bool		token_starts_with(t_token t, char *str);
t_token_old	get_old_token(t_ast_node word);
int			expand_simple_cmd_redir(t_shell *state,
				t_expander_simple_cmd *exp, t_vec_int *redirects);
bool		is_export(t_ast_node word);
bool		is_empty_command(const char *cmd);
void		expand_cmd_substitutions(t_shell *state, t_ast_node *node);
void		expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one,
				bool no_glob);
void		expand_tilde_token(t_shell *state, t_token *t);
char		*expand_word_single(t_shell *state, t_ast_node *curr);
char		*capture_subshell_output(t_shell *state, const char *cmd);
void		replace_trailing_equal_with_full_token(t_ast_node *node,
				t_vec *argv);
int			process_simple_child(t_shell *state, t_expander_simple_cmd *exp,
				t_executable_cmd *ret, t_vec_int *redirects);
void		expand_cmd_substitutions(t_shell *state, t_ast_node *node);

void		process_word_token(t_shell *state, t_token *tok);
bool		try_arith_sub_ctx(t_word_token_ctx *ctx);
bool		try_cmd_sub_ctx(t_word_token_ctx *ctx);
bool		try_backtick_ctx(t_word_token_ctx *ctx);
void		push_single_char_ctx(t_word_token_ctx *ctx);
void		init_word_node(t_ast_node *n);
void		push_token_node(t_ast_node *curr_node, t_ast_node *child);
void		free_token_res(t_token *t);
void		ft_reset(void *ptr, size_t size,
				void (*cust_act_bef_reset)(void *));
void		free_children(void *p);
void		push_and_reinit_curr_node(t_vec_nd *ret, t_ast_node *curr_node);
void		push_new_env_child(t_ast_node *curr_node, char *new_start);
bool		is_ifs_char(char c, const char *ifs);
bool		is_ws_ifs(char c, const char *ifs);
bool		is_nw_ifs(char c, const char *ifs);
void		push_f(t_vec *out, const char *s, size_t start, size_t end);
size_t		skip_ws_delimiter(const char *s, size_t n,
				const char *ifs, size_t i);
bool		process_cmd_sub(t_shell *state, t_expand_ctx *ctx);
bool		process_arith_sub(t_shell *state, t_expand_ctx *ctx);
bool		finish_arith_sub(t_shell *state, t_expand_ctx *ctx, int j);
void		handle_double_close_paren(int *depth, int *j);
void		handle_single_open_paren(int *depth, int *j);
void		handle_single_close_paren(int *depth, int *j);
bool		is_double_open_paren(int slen, const char *s, int j);
bool		is_double_close_paren(int slen, const char *s, int j);
bool		is_single_open_paren(const char *s, int j);
bool		is_single_close_paren(const char *s, int j);
void		handle_double_open_paren(int *depth, int *j);
bool		is_double_close_paren_v1(int slen, const char *s, int j);
bool		is_double_open_paren_v1(int slen, const char *s, int j);

static inline t_expand_ctx	init_expand(const char *s, int slen,
								t_string *outbuf, int *consumed)
{
	t_expand_ctx	ectx;

	ectx.s = s;
	ectx.slen = slen;
	ectx.outbuf = outbuf;
	ectx.consumed = consumed;
	return (ectx);
}

#endif