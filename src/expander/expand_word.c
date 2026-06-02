/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"
#include "decomposer.h"
#include "sys.h"

char	*arith_expand(t_shell *state, const char *expr, int len);

/* A word participates in brace expansion only if it is entirely unquoted
   (all TT_WORD subtokens); quotes/vars inhibit it, matching POSIX shells. */
static bool	word_all_unquoted(t_ast_node *node)
{
	size_t		i;
	int			j;
	t_ast_node	*c;
	bool		has_brace;

	if (!node->children.ctx || node->children.len == 0)
		return (false);
	has_brace = false;
	i = 0;
	while (i < node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i];
		if (c->node_type != AST_TOKEN || c->token.tt != TT_WORD)
			return (false);
		j = 0;
		while (j < c->token.len)
			if (c->token.start[j++] == '{')
				has_brace = true;
		i++;
	}
	return (has_brace);
}

/* Brace-expand `node` into multiple words, re-lexing each result and running
   it through the full expansion pipeline. Returns true if it handled `node`. */
static bool	try_brace_expand(t_shell *state, t_ast_node *node, t_vec *args)
{
	t_string	flat;
	char		*fs;
	t_vec		results;
	t_ast_node	w;
	size_t		i;

	if (!word_all_unquoted(node))
		return (false);
	flat = word_to_string(*node);
	fs = ft_strndup((char *)flat.ctx, flat.len);
	free(flat.ctx);
	if (brace_find_expandable(fs, &(int){0}) < 0)
		return (free(fs), false);
	results = brace_expand_str(fs);
	free(fs);
	i = 0;
	while (i < results.len)
	{
		fs = ((char **)results.ctx)[i++];
		w = reparse_word((t_token){.start = fs, .len = (int)ft_strlen(fs),
				.tt = TT_WORD});
		expand_word(state, &w, args, false);
		free(fs);
	}
	return (free(results.ctx), free_ast(node), true);
}

/* A single literal TT_WORD subtoken with no metacharacter, quote or blank
   expands to itself as exactly one field — no tilde/var/cmdsub/split/glob and
   no clone needed. Lets hot loop words ([ -lt 4000 ]) skip the whole pipeline. */
static bool	word_is_plain_literal(t_ast_node *node)
{
	t_token	*t;
	int		i;
	int		lbr;
	char	c;

	if (node->children.len != 1
		|| ((t_ast_node *)node->children.ctx)[0].node_type != AST_TOKEN)
		return (false);
	t = &((t_ast_node *)node->children.ctx)[0].token;
	if (t->tt != TT_WORD)
		return (false);
	i = 0;
	lbr = -1;
	while (i < t->len)
	{
		c = t->start[i];
		if (c == '$' || c == '`' || c == '*' || c == '?' || c == '~'
			|| c == '{' || c == '\\' || c == '\'' || c == '"'
			|| c == ' ' || c == '\t' || c == '\n')
			return (false);
		if (c == '[')
			lbr = i;
		else if (c == ']' && lbr >= 0 && i > lbr + 1)
			return (false);
		i++;
	}
	return (true);
}

static bool	name_is_plain(const char *s, int len)
{
	int	i;

	if (len <= 0 || !(ft_isalpha(s[0]) || s[0] == '_'))
		return (false);
	i = 1;
	while (i < len)
	{
		if (!(ft_isalnum(s[i]) || s[i] == '_'))
			return (false);
		i++;
	}
	return (true);
}

/* A bare unquoted $var whose value contains no IFS blank and no glob char
   (under the default IFS) expands to exactly that value as one field — no
   split, no glob, no clone. Returns the env value to push, or NULL to fall
   back to the full pipeline. Special params / ${...} forms are excluded. */
/* Does the value contain a char that would trigger field-splitting (default
   IFS) or pathname expansion? Hand-rolled to avoid ft_strcspn building a 32-byte
   bitmap on every hot-loop $var expansion. */
static bool	needs_split_or_glob(const char *v)
{
	while (*v)
	{
		if (*v == ' ' || *v == '\t' || *v == '\n'
			|| *v == '*' || *v == '?' || *v == '[')
			return (true);
		v++;
	}
	return (false);
}

/* The single non-empty sub-token of a word, skipping empty TT_WORD pieces (e.g.
   the empty leading token left on an assignment value `x=$i`). NULL if there are
   two non-empty pieces or any non-token child. */
static t_token	*lone_nonempty_token(t_ast_node *node)
{
	t_token		*t;
	t_ast_node	*c;
	int			i;

	t = NULL;
	i = 0;
	while (i < (int)node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i++];
		if (c->node_type != AST_TOKEN)
			return (NULL);
		if (c->token.tt == TT_WORD && c->token.len == 0)
			continue ;
		if (t)
			return (NULL);
		t = &c->token;
	}
	return (t);
}

static char	*try_simple_envvar(t_shell *state, t_ast_node *node)
{
	t_token	*t;
	char	*ifs;
	char	*v;

	t = lone_nonempty_token(node);
	if (!t || t->tt != TT_ENVVAR || !name_is_plain(t->start, t->len))
		return (NULL);
	ifs = env_get_ifs(&state->env);
	if (ifs && ft_strcmp(ifs, " \t\n") != 0)
		return (NULL);
	v = env_expand_n(state, t->start, t->len);
	if (!v || !*v || needs_split_or_glob(v))
		return (NULL);
	return (v);
}

/* A word that is exactly $((expr)) with no $ or backtick inside is pure
   arithmetic: evaluate it directly (no clone, no split/glob — the result is a
   number). Returns the result string (caller owns), or NULL to fall back.
   Words containing $ or ` need the full pipeline (nested cmdsub / $var refs). */
/* The single non-empty TT_WORD child of `node` (skipping empty tokens, e.g. the
   empty leading token of an assignment value), or NULL if the word isn't a lone
   bare word (any quoted/var subtoken or two non-empty pieces disqualify it). */
static t_token	*lone_word_token(t_ast_node *node)
{
	t_token		*t;
	t_ast_node	*c;
	int			i;

	t = NULL;
	i = 0;
	while (i < (int)node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i++];
		if (c->node_type != AST_TOKEN || c->token.tt != TT_WORD)
			return (NULL);
		if (c->token.len == 0)
			continue ;
		if (t)
			return (NULL);
		t = &c->token;
	}
	return (t);
}

/* A word that is exactly $((expr)) with no $ or backtick inside is pure
   arithmetic: evaluate directly (no clone, no split/glob — result is a number).
   Returns the result string (caller owns) or NULL to fall back. */
static char	*try_pure_arith(t_shell *state, t_ast_node *node)
{
	t_token	*t;
	int		i;

	t = lone_word_token(node);
	if (!t || t->len < 5 || t->start[0] != '$' || t->start[1] != '('
		|| t->start[2] != '(' || t->start[t->len - 1] != ')'
		|| t->start[t->len - 2] != ')')
		return (NULL);
	i = 3;
	while (i < t->len - 2)
	{
		if (t->start[i] == '$' || t->start[i] == '`')
			return (NULL);
		i++;
	}
	return (arith_expand(state, t->start + 3, t->len - 5));
}

/* Non-destructive variant: expand the words of `src` into `args` without
   mutating or freeing `src`. Fast-paths plain literals, simple $var and pure
   $((arith)) so hot loop words never clone; falls back to a private clone + the
   destructive pipeline for everything else, leaving the caller's `src` intact. */
void	expand_word_ro(t_shell *state, t_ast_node *src,
					t_vec *args, bool keep_as_one)
{
	t_ast_node	scratch;
	t_token		*t;
	char		*v;

	if (word_is_plain_literal(src))
	{
		t = &((t_ast_node *)src->children.ctx)[0].token;
		return ((void)vec_push(args, &(char *){ft_strndup(t->start, t->len)}));
	}
	v = try_pure_arith(state, src);
	if (v)
		return ((void)vec_push(args, &(char *){v}));
	v = try_simple_envvar(state, src);
	if (v)
		return ((void)vec_push(args, &(char *){ft_strdup(v)}));
	scratch = clone_ast(src);
	expand_word(state, &scratch, args, keep_as_one);
}

void	expand_word(t_shell *state, t_ast_node *node,
					t_vec *args, bool keep_as_one)
{
	t_vec_nd	words;
	size_t		i;

	if (!keep_as_one && try_brace_expand(state, node, args))
		return ;
	if (!node->children.ctx || node->children.len == 0)
	{
		vec_push(args, &(char *){ft_strdup("")});
		return (free_ast(node));
	}
	(expand_tilde_word(state, node), expand_cmd_substitutions(state, node));
	(expand_env_vars(state, node, !keep_as_one), vec_init(&words));
	words.elem_size = sizeof(t_ast_node);
	if (!keep_as_one)
		words = split_words(state, node);
	else
	{
		vec_push(&words, node);
		*node = (t_ast_node){};
	}
	i = -1;
	while (++i < words.len)
	{
		expand_node_glob(&((t_ast_node *)words.ctx)[i], args, keep_as_one);
		if (get_g_sig()->should_unwind)
			while (i < words.len)
				free_ast(&((t_ast_node *)words.ctx)[i++]);
		if (get_g_sig()->should_unwind)
			break ;
	}
	(free(words.ctx), free_ast(node));
	return ;
}

char	*expand_proc_sub(t_shell *state, t_ast_node *node)
{
	t_token		*tok;
	t_ast_node	*cmd_word;
	t_string	cmd_str;
	char		*result;
	bool		is_input;

	if (!node || node->node_type != AST_PROC_SUB || node->children.len < 2)
		return (NULL);
	tok = &((t_ast_node *)node->children.ctx)[0].token;
	cmd_word = &((t_ast_node *)node->children.ctx)[1];
	is_input = (tok->tt == TT_PROC_SUB_IN);
	cmd_str = word_to_string(*cmd_word);
	if (!cmd_str.ctx)
		return (ft_strdup(BLACK_HOLE));
	if (!vec_ensure_space_n(&cmd_str, 1))
		return (free(cmd_str.ctx), NULL);
	((char *)cmd_str.ctx)[cmd_str.len] = '\0';
	if (is_input)
		result = create_procsub_input(state, (char *)cmd_str.ctx);
	else
		result = create_procsub_output(state, (char *)cmd_str.ctx);
	free(cmd_str.ctx);
	return (result);
}
