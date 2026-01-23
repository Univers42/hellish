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
bool		create_redir(t_tt tt, char *fname, t_redir *ret, int src_fd);
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
void		expand_token(t_shell *state, t_token	*curr_tt);
void		expand_env_vars(t_shell *state, t_ast_node *node);
t_ast_node	new_env_node(char *new_start);
void		split_envvar(t_shell *state, t_token *curr_t,
				t_ast_node *curr_node, t_vec_nd *ret);
t_vec_nd	split_words(t_shell *state, t_ast_node *node);
t_ast_node	new_env_node(char *new_start);
bool		token_starts_with(t_token t, char *str);
t_token_old	get_old_token(t_ast_node word);
int			expand_simple_cmd_redir(t_shell *state,
				t_expander_simple_cmd *exp, t_vec_int *redirects);
bool		is_export(t_ast_node word);
bool		is_empty_command(const char *cmd);
void		expand_cmd_substitutions(t_shell *state, t_ast_node *node);
void		expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one);
void		expand_tilde_token(t_shell *state, t_token *t);
char		*expand_word_single(t_shell *state, t_ast_node *curr);
char		*capture_subshell_output(t_shell *state, const char *cmd);
void		replace_trailing_equal_with_full_token(t_ast_node *node,
				t_vec *argv);
int			process_simple_child(t_shell *state, t_expander_simple_cmd *exp,
				t_executable_cmd *ret, t_vec_int *redirects);
void		expand_cmd_substitutions(t_shell *state, t_ast_node *node);

void		process_word_token(t_shell *state, t_token *tok);
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