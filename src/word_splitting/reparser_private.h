/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparser_private.h                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 21:46:13 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Internal types and prototypes for the word-splitting "second parse pass".
   After the lexer produces flat TT_WORD tokens, the reparser walks each one
   character by character and decomposes it into typed subtokens: TT_SQWORD
   (literal), TT_DQWORD (literal, no IFS split), TT_ENVVAR / TT_DQENVVAR
   (variable expansion), TT_WORD (split-eligible plain text or $(...)).
   t_reparser is the shared state threaded through every recursive helper so
   we avoid passing five arguments to every function. t_paren_ctx bundles the
   same state specifically for the $(...) / $((...)) sub-scanner. */
#ifndef REPARSER_PRIVATE_H
# define REPARSER_PRIVATE_H

# include "shell.h"
# include <stdbool.h>
# include "libft.h"
# include <stddef.h>
# include "parser.h"
# include "decomposer.h"
# include "lexer.h"
# include "ast.h"
# include "helpers.h"

typedef struct s_paren_ctx
{
	t_ast_node	*ret;
	int			*i;
	t_token		t;
	int			prev_start;
	t_tt		tt;
}	t_paren_ctx;

typedef struct s_interval
{
	int	start;
	int	end;
}	t_interval;

typedef struct s_reparser
{
	t_ast_node	current_node;
	int			i;
	t_token		current_token;
	t_tt		ctx_tt;
	int			prev_start;
	bool		no_squote;
}	t_reparser;

/* Decode state for $'...' ANSI-C quoting (reparse_ansic*.c): the source
   token text, a read cursor, and the write cursor into the decode buffer. */
typedef struct s_ansic
{
	const char	*s;
	int			len;
	int			i;
	char		*dst;
	int			n;
}	t_ansic;

void		reparse_dq_bs(t_ast_node *ret, int *i, t_token t);
void		reparse_squote(t_ast_node *ret, int *i, t_token t);
void		reparse_bs(t_ast_node *ret, int *i, t_token t);
void		reparse_norm_word(t_ast_node *ret, int *i, t_token t);
t_ast_node	reparse_word(t_token t, bool no_squote);
bool		reparse_special_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt);
void		reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt);
void		reparse_dquote(t_ast_node *ret, int *i, t_token t);
void		push_subtoken_node(t_ast_node *ret, t_token t,
				t_interval offset,
				t_tt tt);
bool		is_valid_ident(char *s, int len);
void		push_dqword_subtoken_rp(t_reparser *rp, int start, int end);
void		flush_pending_segment_rp(t_reparser *rp, bool *pushed_any);
void		process_dollar_in_dquote_rp(t_reparser *rp, bool *pushed_any);
void		process_escape_seq_rp(t_reparser *rp, bool *pushed_any);
void		reparse_envvar_paren(t_paren_ctx ctx);
void		reparse_backtick(t_ast_node *ret, int *i, t_token t);
bool		is_double_open_paren(t_token t, int idx);
bool		is_double_close_paren(t_token t, int idx);
bool		is_open_paren(t_token t, int idx);
bool		is_close_paren(t_token t, int idx);
bool		try_handle_paren_rp(t_reparser *rp, int prev_start, t_tt tt);
bool		try_handle_special_rp(t_reparser *rp, t_tt tt);
void		consume_ident_rp(t_reparser *rp);
t_tt		select_literal_tt(t_tt ctx_tt, t_token *t, int prev_start);
void		handle_envvar_literal(t_reparser *rp, int prev_start, t_tt tt);
bool		handle_envvar_empty(t_reparser *rp, int prev_start, t_tt tt);
void		handle_envvar_quote(t_reparser *rp, int prev_start, t_tt tt);
void		update_envvar_result(t_ast_node *ret, int *i, t_reparser *rp);
void		handle_envvar_ident(t_reparser *rp, int prev_start, t_tt tt);
bool		handle_envvar_paren_or_special(t_reparser *rp,
				int prev_start, t_tt tt);
void		skip_quoted_in_brace(t_reparser *rp, char q);
void		loop_node_rp(t_reparser *rp);
void		reparse_ansic(t_ast_node *ret, int *i, t_token t);
void		reparse_subscript_assigns(t_ast_node *node);
void		ansic_escape(t_ansic *a);
int			ansic_simple(char c);
int			ansic_digit(char c, char kind);
void		ansic_utf8(t_ansic *a, long cp);
void		ansic_num(t_ansic *a, char kind);

static inline t_interval	create_interval(int start, int end)
{
	t_interval	interval;

	interval.start = start;
	interval.end = end;
	return (interval);
}

static inline void	create_reparser(t_reparser *rp,
								t_ast_node current_node,
								t_token current_token,
								int *i)
{
	rp->current_node = current_node;
	rp->current_token = current_token;
	rp->i = *i;
	rp->ctx_tt = current_token.tt;
	rp->prev_start = 0;
}

/* zsh's `$+name[key]` / `$name[key]` (reparse_zsh.c). Gated on the dialect
   latch reparse_all sets, so bash input cannot reach it. */
bool		reparse_zsh_param(t_reparser *rp, int prev_start, t_tt tt);

#endif