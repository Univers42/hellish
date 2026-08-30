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

/* Private header for the expansion engine: everything the expander .c files
   share with each other but should not leak into the rest of the shell.
   Contexts, helpers, and the inline init are all here so each .c stays small.

   Quick map of the expansion pipeline (left to right, order matters):
     1. Brace expansion   – {a,b,c}  {1..5}  (try_brace_expand)
     2. Tilde             – ~ ~+ ~-  (expand_tilde_word)
     3. Parameter / $     – $x ${x:-y} ${#x} ${x%p} ${x/p/r}
                            (expand_token -> expand_param_format)
     4. Command subst.    – $(...) `...`  (process_cmd_sub / try_backtick_ctx)
     5. Arithmetic        – $((...))      (process_arith_sub)
     6. Field splitting   – IFS split on unquoted expansions (split_words)
     7. Pathname (glob)   – * ? [...]   (expand_node_glob / expand_word_glob)

   Quoting ('' "" \) is tracked by the token type (TT_SQWORD, TT_DQWORD …)
   so quoted pieces never reach field-splitting or globbing.              */

#ifndef EXPANDER_PRIVATE_H
# define EXPANDER_PRIVATE_H

# include "libft.h"
# include "shell.h"
# include "pal.h"
# include <stdio.h>
# include "env.h"
# include "expander.h"
# include <fcntl.h>
# include <stdbool.h>
# include "ft_glob.h"
# include "helpers.h"
# include "pal_wait.h"
# include <stdlib.h>
# include <string.h>
# include "arith.h"
# include "pal_wait.h"
# include <errno.h>
# include <unistd.h>
# include "pal_wait.h"
# include <signal.h>

/* Carry-along for $((expr)) expansion: the shell state, the expression text,
   its length, a write cursor, and the output buffer. Kept in a struct so all
   the arith helper functions share a single pointer rather than five args. */
typedef struct s_arith_expand_ctx
{
	t_shell		*state;
	const char	*s;
	int			len;
	int			*i;
	t_string	*out;
}	t_arith_expand_ctx;

/* Brace-sequence descriptor: {1..5}, {a..z}, {01..10..2} etc.
   `alpha` is true for single-character ranges; `width` is the zero-pad
   target computed from leading zeros in the endpoints. */
typedef struct s_seq
{
	long	a;
	long	b;
	long	step;
	int		width;
	bool	alpha;
}	t_seq;

/* Bundled arguments for the per-word glob pass so glob_loop() does not need
   a six-argument signature.  `keep_as_one` suppresses IFS splitting
   (double-quoted context); `no_glob` suppresses pathname expansion (set -f
   or an assignment value). */
typedef struct s_expand_glob_ctx
{
	t_shell		*state;
	t_ast_node	*node;
	t_vec		*args;
	bool		keep_as_one;
	bool		no_glob;
}	t_expand_glob_ctx;

/* Context for ${name#pat}, ${name%pat}, ${name/pat/rep}: both the variable
   name slice and the full raw spec inside the braces, so helpers can
   re-expand the pattern without re-parsing. */
typedef struct s_trim_ctx
{
	const char	*name;
	int			name_len;
	const char	*op;
	int			slen;
}	t_trim_ctx;

/* Parsed ${name[:]opc word} operator (the four modifying forms: - = ? +).
   `colon` means the colon-variant was used (triggers on null as well as
   unset); `opc` is the operator character; `word` and `wlen` point into the
   raw braced text and are re-expanded lazily when actually needed.  `dq` is
   true when the whole ${...} sits inside double quotes: the word then keeps
   double-quote backslash semantics ("${u-\z}" prints \z, not z).  Only
   expand_op_token knows the enclosing token type, so find_param_op defaults
   it to false and expand_op_token overrides. */
typedef struct s_pe_op
{
	const char	*name;
	int			name_len;
	char		opc;
	bool		colon;
	const char	*word;
	int			wlen;
	bool		dq;
}	t_pe_op;

/* Slice of the raw token text fed to process_cmd_sub / process_arith_sub.
   `consumed` is written back to report how many bytes were eaten so the
   caller can advance its scan position. */
typedef struct s_expand_ctx
{
	const char	*s;
	int			slen;
	t_string	*outbuf;
	int			*consumed;
}	t_expand_ctx;

/* Mutable scan state shared by the try_*_ctx helpers while a single TT_WORD
   token is walked for $(...) / $((...)) / `...` expansions.  `changed` is
   set when at least one substitution occurred so the final update knows to
   build a new allocated string. */
typedef struct s_word_token_ctx
{
	t_shell		*state;
	t_token		*tok;
	t_string	*outbuf;
	int			total_len;
	int			pos;
	bool		changed;
}	t_word_token_ctx;

/* Position cookie passed to process_env_token so it can peek at the sibling
   token: a lone $ followed by '' or "" should produce "$" literally, not "".
   split_ctx mirrors expand_env_vars's arg so the "$@" special case works. */
typedef struct s_env_tok_pos
{
	size_t	idx;
	bool	split_ctx;
}	t_env_tok_pos;

t_string	word_to_string(t_ast_node node);
t_string	word_to_brace_src(t_ast_node node);
t_string	word_to_hrdoc_string(t_ast_node node);
t_env		assignment_to_env(t_shell *state, t_ast_node *node);
t_env		assignment_to_env(t_shell *state, t_ast_node *node);
void		assignment_word_to_word(t_ast_node *node);
void		procsub_exec_self(t_shell *state, const char *cmd);
char		*create_procsub_input(t_shell *state, const char *cmd);
char		*create_procsub_output(t_shell *state, const char *cmd);
char		*create_procsub_file(t_shell *state, const char *cmd);
char		*expand_proc_sub(t_shell *state, t_ast_node *node);
void		procsub_close_fds_parent(t_shell *state);
void		cleanup_proc_subs(t_shell *state);
void		procsub_detach_all(t_shell *state);
int			net_redir_open(char *fname, t_redir *ret);
bool		create_redir_4(t_tt tt, char *fname, t_redir *ret, int src_fd);
bool		redir_park_fd(t_redir *ret);
bool		dup_target_is_fd(const char *fname);
int			parse_src_fd(t_tt tt, t_token op_tok);
int			try_create_redir(t_shell *state, t_ast_node *curr,
				t_tt tt, int src_fd);
char		*expand_param_word(t_shell *state, const char *word, int wlen,
				bool dq);
char		*expand_param_word_dq(t_shell *state, const char *word, int wlen);
char		*pf_word_pipeline(t_shell *state, const char *word, int wlen,
				bool no_sq);
t_string	word_to_pattern(t_ast_node node);
char		*expand_param_pattern(t_shell *state, const char *word, int wlen);
char		*pf_get_var_value(t_shell *state, const char *name, int len);
char		*expand_strlen(t_shell *state, const char *s, int slen);
char		*default_or_alt(t_shell *state, char *val, t_pe_op o);
char		*expand_case(t_shell *state, const char *s, int slen, int name_len);
bool		find_case_op(const char *s, int slen, int *nl);
char		*expand_xform(t_shell *state, const char *s, int name_len, char op);
bool		find_xform_op(const char *s, int slen, int *nl, char *op);
bool		pf_is_indirect(const char *s, int n);
char		*expand_indirect(t_shell *state, const char *s, int n);
char		*err_or_assign(t_shell *state, char *val, t_pe_op o);
char		*expand_param_op(t_shell *state, t_pe_op o);
char		*pf_err_word(t_shell *state, char *val, t_pe_op o);
char		*pf_assign_err(t_shell *state, t_pe_op o);
bool		expand_op_token(t_shell *state, t_token *tt, bool split_ctx);
void		expand_positional_op(t_shell *state, t_token *tt, t_pe_op o,
				bool split_ctx);
bool		find_param_op(const char *s, int slen, t_pe_op *o);
int			pf_scan_name(const char *s, int slen);
bool		pat_match_pub(const char *p, const char *s);
char		*trim_suffix_shortest(const char *val, const char *pattern);
char		*trim_suffix_longest(const char *val, const char *pattern);
char		*trim_prefix_shortest(const char *val, const char *pattern);
char		*trim_prefix_longest(const char *val, const char *pattern);
char		*expand_trim(t_shell *state, t_trim_ctx ctx);
char		*expand_subst(t_shell *state, t_trim_ctx ctx);
int			patsub_match_len(const char *pat, const char *s);
int			subst_anchor(t_trim_ctx ctx, int g);
char		*patsub_prefix(const char *val, const char *pat, const char *rep);
char		*patsub_suffix(const char *val, const char *pat, const char *rep);
bool		pf_valid_plain(const char *s, int n);
char		*pf_trim_or_subst(t_shell *state, t_trim_ctx ctx);
t_trim_ctx	pf_make_ctx(const char *s, int slen, const char *op, int nlen);
char		*cmdsub_fast(t_shell *state, const char *cmd);
bool		csf_eligible(t_shell *state, const char *s);
bool		cs_single_cmd(t_shell *state, const char *s);
int			csf_skip_quoted(const char *s, int i);
int			csf_skip_csub(const char *s, int i);
int			csf_skip_param(const char *s, int i);
char		*pf_bad_subst(t_shell *state, const char *s, int slen);
bool		pf_find_substr(const char *s, int slen, int *name_len);
char		*expand_substr(t_shell *state, const char *s, int slen,
				int name_len);
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
void		split_value(t_shell *state, const char *val,
				t_ast_node *curr_node, t_vec_nd *ret);
const char	*pos_mark(char which);
void		emit_positional_split(t_shell *state, t_ast_node *curr_node,
				t_vec_nd *ret);
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

/* indexed arrays (expand_array*.c, split_array.c) */
bool		expand_array_token(t_shell *state, t_token *tt, bool split_ctx);
char		*arr_mark_push(t_shell *state, const char *name, int nlen);
char		*arr_mark_name(t_shell *state, const char *p);
void		arr_marks_clear(t_shell *state);
bool		arr_keys_defer(t_shell *state, t_token *tt);
void		emit_keys_fields(t_shell *state, const char *name,
				t_ast_node *curr_node, t_vec_nd *ret);
void		emit_array_at(t_shell *state, const char *name,
				t_ast_node *curr_node, t_vec_nd *ret);
void		emit_array_split(t_shell *state, const char *name,
				t_ast_node *curr_node, t_vec_nd *ret);
void		emit_assoc_fields(char *val, t_ast_node *curr_node, t_vec_nd *ret,
				int want_keys);
typedef struct s_slice_ctx
{
	char	*val;
	int		n;
	int		off;
	int		lim;
}	t_slice_ctx;

bool		expand_array_ext(t_shell *state, t_token *tt, bool split_ctx);
bool		expand_pos_slice(t_shell *state, t_token *tt);
bool		expand_array_op(t_shell *state, t_token *tt);
bool		expand_array_elem_op(t_shell *state, t_token *tt);
bool		arr_keys(t_shell *state, t_token *tt, bool split_ctx);
bool		arr_prefix_names(t_shell *state, t_token *tt, bool split_ctx);
bool		arr_emit(t_token *tt, char *owned);
bool		at_op_ok(const char *op, int oplen);
bool		arr_slice(t_shell *state, t_token *tt, int nl, int colon);
char		*idx_str(long idx);
int			arith_num(t_shell *state, const char *s, int len);
int			slice_off_len(const char *s, int len, int first);
int			slice_name_len(const char *s, int len, int *colon);
char		*arr_slice_build(t_slice_ctx *c);
int			herestring_redir(t_shell *state, t_ast_node *curr, int src_fd);
int			handle_array_assign(t_shell *state, t_expander_simple_cmd *exp,
				t_executable_cmd *ret);
int			is_assign_builtin(t_executable_cmd *ret);
int			parse_sub_elem(char *elem, char **sub, int *subl, char **val);
int			has_subscript(t_vec *args);

/* One compound assignment name=(...) being applied: the half-built env
   entry (key already stripped of any +=), the expanded element strings,
   and whether the spelling was +=. Bundled so build_array_value keeps a
   four-argument signature (same move as t_expand_glob_ctx above). */
typedef struct s_arr_assign
{
	t_env	*ev;
	t_vec	*args;
	int		append;
}	t_arr_assign;

char		*build_array_value(t_shell *state, t_executable_cmd *ret,
				t_arr_assign *aa);
char		*build_indexed_sub(t_shell *state, t_vec *args,
				const char *base, int append);
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

/* One parsed ${(flags)x} prefix -- the zsh dialect only, gated on zsh_mode()
   at the single entry point (expand_zsh_flags).  `set` is the letters seen,
   NUL-terminated, so zf_has() is a strchr over at most a dozen bytes: a flag
   list is never long enough for anything cleverer to pay for itself.
   `sep` and `join` hold the arguments of (s:x:) and (j:x:), already
   unescaped when (p) asked for it. */
# define ZF_MAX 23

typedef struct s_zflags
{
	char	set[ZF_MAX + 1];
	int		n;
	char	*sep;
	char	*join;
	bool	split;
	bool	array;
}	t_zflags;

bool		expand_zsh_flags(t_shell *state, t_token *tt, bool split_ctx);
bool		zsh_bare_nested(t_shell *state, t_token *tt, bool split_ctx);
bool		zf_arrayness(t_zflags *f, t_token *tt, const char *s, int slen);
bool		zf_has(const t_zflags *f, char c);
int			zf_count(const t_zflags *f, char c);
char		zq_style(const t_zflags *f);
void		zf_free(t_zflags *f);
char		*zf_inner(t_shell *state, t_token *tt, const char *s, int slen);
bool		zf_is_nested(const char *s, int slen);
int			zn_at_len(const char *s, int slen);
int			zn_sub_len(const char *s, int slen);
char		*zn_subscript(t_shell *state, char *enc, const char *s, int slen);
char		*zsh_strlen(t_shell *state, const char *s, int slen);
char		*zsh_param(t_shell *state, const char *s, int slen);
/* zsh modifiers ${x:h} ${x:t} ... (expand_zsh_mod*.c) and the :# filter
   operator with its (M) partner (expand_zsh_hash.c). */
char		*zsh_dispatch(t_shell *state, const char *s, int slen, bool arr);
char		*zf_inner_text(t_shell *state, const char *s, int n);
char		*zsh_token_text(t_shell *state, const char *s, int slen);
char		*zd_plain(t_shell *state, const char *name, int len);
char		*zd_splice(const char *val, const char *rest, int rlen);
char		*zd_bind_name(int depth);
void		zd_unbind(t_shell *state, const char *name);
char		*zsh_modifier(t_shell *state, const char *s, int slen);
/* ${x:#pat}: `want` inverts it (the (M) flag), `as_array` says whether the
   expansion is still an array at this point -- quoting joins first, which
   changes the answer. */
typedef struct s_zhash
{
	bool	want;
	bool	as_array;
}	t_zhash;

char		*zsh_hash_op(t_shell *state, const char *s, int slen, t_zhash h);
int			zh_find(const char *s, int slen, int *name_len);
char		*zm_head(const char *v);
char		*zm_tail(const char *v);
char		*zm_root(const char *v);
char		*zm_ext(const char *v);
char		*zm_abs(t_shell *state, const char *v);
bool		zsh_param_token(t_shell *state, t_token *tt);
bool		zsh_hash_token(t_shell *state, t_token *tt);
char		*zp_which(t_shell *state, char *name);
int			zp_elem_set(t_shell *state, const char *base, int blen, char *key);
int			zsh_len_span(t_shell *state, const char *s, int len, int j);
bool		append_zsh_len(t_arith_expand_ctx *ctx);
long		elem_sub_index(t_shell *state, char *text, long count);
bool		splice_elem_assign(t_shell *state, t_env *ev, t_vec *args);
void		bad_subscript(t_shell *state, const char *name, const char *sub);
t_slice		zsh_slice_bounds(t_shell *state, const char *body, int blen,
				const char *val);
long		zsh_slice_universe(const char *val);
long		zsh_slice_len(t_slice r, long count);
char		*zsh_slice_str(t_shell *state, const char *val, t_slice r);
void		zsh_slice_set(t_env *ret, const char *old, t_slice r);
void		assoc_elem_assign(t_shell *state, t_env *ret, char *sub,
				char *old);
char		*zn_scalar_pick(t_shell *state, const char *val, long sub);
char		*zf_nested(t_shell *state, t_token *tt, const char *s, int slen);
int			zf_parse(const char *s, int slen, t_zflags *f);
void		zf_bad(t_shell *state, t_token *tt, char flag);
bool		zf_check(t_shell *state, t_zflags *f, t_token *tt);
bool		zf_hash_form(t_shell *state, t_zflags *f, t_token *tt, int end);
void		zf_unesc(t_zflags *f);
void		zf_finish(t_shell *state, t_zflags *f, t_token *tt, char *val);
void		zf_install(t_token *tt, char *owned);
void		zf_emit(t_shell *state, t_zflags *f, t_token *tt, t_vec *l);
void		zf_emit_value(t_shell *state, t_zflags *f, t_token *tt, char *val);
char		*zl_join(t_vec *l, const char *sep);
void		zl_push(t_vec *l, char *owned);
void		zl_free(t_vec *l);
void		zl_erase(t_vec *l, size_t at);
void		zl_swap(char **a, char **b);
t_vec		zl_from(t_shell *state, t_zflags *f, const char *val);
void		zl_split_ws(t_vec *l, const char *v);
void		zl_records(t_shell *state, t_zflags *f, t_vec *l, const char *val);
void		zl_arr_one(t_zflags *f, t_vec *l, long idx, char *owned);
void		zl_order(t_zflags *f, t_vec *l);
void		zl_uniq(t_zflags *f, t_vec *l);
void		zl_map(t_shell *state, t_zflags *f, t_vec *l);
char		*zx_one(t_shell *state, t_zflags *f, char *v);
char		*zf_quote(const char *v, char style);
char		*zf_case(const char *v, char how);
void		emit_val_at(const char *val, t_ast_node *curr_node, t_vec_nd *ret);

/* Convenience inline: zero-initialise a t_expand_ctx from its four fields.
   Inlined so there is zero overhead when called from tight scan loops. */
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

/* subscript_append.c: `a[i]+=v` / `M[k]+=v` -- splice the element's current
   value in front of the new one, then let the ordinary set path store it. */
long		arr_sub_index(t_shell *state, const char *sub, long count);
bool		subscript_take_append(t_env *ret);
void		subscript_append_value(t_shell *state, t_env *ret,
				const char *sub, const char *old);
void		subscript_prepend_current(t_shell *state, t_env *ret, char *br);

/* procsub_assign.c: a process substitution glued to the assignment word
   before it (`x=<(cmd)`, zsh's `x==(cmd)`) is that assignment's VALUE, not
   a separate operand. */
bool		procsub_is_assign_rhs(t_expander_simple_cmd *exp);
void		procsub_join_assign(t_expander_simple_cmd *exp,
				t_executable_cmd *ret, char *path);

#endif
