/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MINISHELL_H
# define MINISHELL_H

// debug printing of the ast
# ifndef PRINT_AST
#  define PRINT_AST 0
# else
#  define PRINT_AST 1
# endif

// debug printing of the token queue
# ifndef PRINT_TOKENS
#  define PRINT_TOKENS 0
# else
#  define PRINT_TOKENS 1
# endif

# define TRACE_DEBUG 0

# include "libft/libft.h"
# include "libft/dsa/vec_int.h"
# include <dirent.h>
# include <stdbool.h>
# include <stdlib.h>
# include <stdbool.h>

# define COMMAND_NOT_FOUND 127
# define EXE_PERM_DENIED 126
# define EXE_BASE_ERROR 125
# define CANCELED 130
# define SYNTAX_ERR 2
# define AMBIGUOUS_REDIRECT 1
# define PROMPT "\001❯ \002"
# define HIST_FILE ".minishell_history"

# define ANSI_RED "\001\033[31m\002"
# define ANSI_GREEN "\001\033[32m\002"
# define ANSI_RESET "\001\033[0m\002"
# define RL_SPACER_1 "\x03"

# define LEXER_SQUOTE_PROMPT "squote> "
# define LEXER_DQUOTE_PROMPT "dquote> "

extern uint32_t	g_should_unwind;

typedef struct s_state t_state;

typedef enum e_glob
{
	G_PATTERN,
	G_SLASH,
	G_ASTERISK
}	t_glob_tt;

typedef struct s_glob
{
	t_glob_tt	ty;
	char		*start;
	int			len;
}	t_glob;

typedef struct s_vec
{
	size_t	cap;
	size_t	len;
	size_t	elem_size;
	void	*ctx;
}	t_vec;

# include "dsa/hash.h"

typedef t_vec	t_vec_glob;		// where ctx = sizeof(t_glob)
typedef t_vec	t_string;



typedef struct s_exe_res
{
	int	status;
	int	pid;
	bool	c_c;
}	t_exe_res;

typedef t_vec t_vec_exe_res;

typedef struct redir_s
{
	bool		direction_in;
	int			fd;
	char		*fname;
	bool		should_delete;
}	t_redir;

// the void *ctx willl hold the t_redir
typedef t_vec t_vec_redir;

bool	vec_init(t_vec *ret);
bool	vec_ensure_space(t_vec *v);
bool	vec_ensure_space_n(t_vec *v, size_t n);
bool	vec_push(t_vec *v, const void *el);
bool	vec_push_nstr(t_vec *v, const char *str, size_t n);
bool vec_push_str(t_vec *v, const char *str);
bool	vec_push_byte(t_vec *v, unsigned char byte);
void	*vec_pop(t_vec *v);
void	*vec_idx(t_vec *v, size_t idx);
void	*vec_ctx_idx(t_vec *v, size_t idx);
void vec_append_fd(int fd, t_vec *ret);
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
typedef enum s_res_t
{
	RES_OK,
	RES_FatalError,
	RES_MoreInput,
	RES_Init,
}	t_res_t;

typedef struct s_parser
{
	t_res_t		res;
	t_vec_int	parse_stack;
}	t_parser;

// buff_readline.c
typedef struct s_buff_readline
{
	bool		has_line;
	bool		should_update_context;
	bool		has_finished;
	int			line;
	t_string	buff;
	size_t		cursor;
}	t_buff_readline;

typedef enum e_tt
{
	TT_NONE = 0,
	TT_WORD,			// asfkaslfkj
	TT_REDIRECT_LEFT,	// <
	TT_REDIRECT_RIGHT,	// >
	TT_APPEND,			// >>
	TT_PIPE,			// |
	TT_BRACE_LEFT,		// (
	TT_BRACE_RIGHT,		// )
	TT_OR,				// ||
	TT_AND,				// &&
	TT_SEMICOLON,		// ;
	TT_HEREDOC,			// << | <<-
	TT_NEWLINE,			// '\n'
	TT_END,
	TT_SQWORD,
	TT_DQWORD,
	TT_ENVVAR,
	TT_DQENVVAR,
}	t_tt;

typedef struct s_token_old
{
	bool	present;
	char	*start;
	int		len;
}	t_token_old;

typedef struct s_token
{
	char		*start;
	int			len;
	t_tt		tt;
	t_token_old	full_word;
	bool		allocated;
}	t_token;

typedef enum e_ast_t
{
	AST_SIMPLE_LIST,
	AST_COMMAND_PIPELINE,
	AST_REDIRECT,
	AST_SIMPLE_COMMAND,
	AST_SUBSHELL,
	AST_COMPOUND_LIST,
	AST_COMMAND,
	AST_WORD,
	AST_ASSIGNMENT_WORD,
	AST_TOKEN,
}	t_ast_t;

// void * = > struct s_ast_node
typedef t_vec	t_vec_nd;

typedef struct s_ast_node
{
	t_ast_t		node_type;
	t_token		token;
	bool		has_redirect;
	int			redir_idx;
	t_vec_nd	children;
}	t_ast_node;

typedef struct s_deque_s
{
	size_t	cap;
	size_t	len;
	size_t	start;
	size_t	end;
	size_t	elem_size;
	void	*buff;
}	ft_deque;

/* NOTE: change deque API to accept a generic pointer (void *) so callers
   may pass either ft_deque* or a wrapper t_deque_tt* whose first member is
   an ft_deque. This preserves existing call sites. */
bool	ft_deque_init(void *ret, int size, size_t elem_size);
void	ft_deque_ensure_space(void *ret);
bool	ft_deque_push_end(void *ret, const void *item);
bool	ft_deque_push_start(void *ret, const void *item);
void	*ft_deque_pop_end(void *ret);
void	*ft_deque_pop_start(void *ret);
void	*ft_deque_idx(void *ret, size_t idx);
void	*ft_deque_peek(void *ret);
void	ft_deque_peek_into(void *ret, void *out);
void	ft_deque_reset(void *ret);
void	ft_deque_clone(void *ret, const void *proto);
void	ft_deque_destroy(void *ret, void (*free_elem)(void *));
void	ft_deque_clear(void *ret, void (*free_elem)(void *));

typedef struct s_deque_tt
{
	ft_deque	deqtok;	// void *buff ==> t_token 
	char		looking_for;
}	t_deque_tt;

enum e_input_method
{
	INP_READLINE,
	INP_FILE,
	INP_ARG,
	INP_STDIN_NOTTY,
};

typedef struct s_env
{
	bool	exported;
	char	*key;
	char	*value;
}	t_env;

typedef t_vec t_vec_env;

typedef struct s_history
{
	bool		hist_active;
	int			append_fd;
	t_vec	hist_cmds;
}	t_history;


struct s_state
{
	t_string		input;
	t_vec_env		env;
	t_string		cwd;
	t_ast_node		tree;
	int				input_method;
	char			*base_context;
	char			*context;
	char			*pid;
	char			*last_cmd_status_s;
	t_exe_res		last_cmd_status_res;
	t_history		hist;
	bool			should_exit;
	t_vec_redir		redirects;
	int				heredoc_idx;
	t_buff_readline	readline_buff;
	t_prng_state	prng;
};


typedef t_vec	t_vec_env;		// where ctx = sizeof(t_env)
// get next line (without \n).
// Appends the line to ret
//
// return value:
// 0 - on no input (ctrl-d)
//
// 1 - on empty line
//
// 2 - ctrl-c
//
// 3 - anything else
int			buff_readline(t_state *state, t_string *ret, char *prompt);
void		buff_readline_update(t_buff_readline *l);
void		buff_readline_reset(t_buff_readline *l);
int			get_more_input_readline(t_buff_readline *l, char *prompt);
void		update_context(t_state *state);
int			get_more_input_notty(t_state *state);

typedef struct executable_cmd_s
{
	t_vec_env	pre_assigns;
	t_vec	argv;
	char		*fname;
}	t_executable_cmd;

/* lexer.c */
char		*tokenizer(char *str, t_deque_tt *ret);
int			advance_dquoted(char **str);
int			advance_squoted(char **str);
void		free_all_state(t_state *state);

// parser.c
t_ast_node	parse_pipeline(t_state *state, t_parser *parser,
				t_deque_tt *tokens);
t_ast_node	parse_tokens(t_state *state, t_parser *res,
				t_deque_tt *tokens);
t_ast_node	parse_simple_list(t_state *state, t_parser *parser,
				t_deque_tt *tokens);
t_ast_node	parse_command(t_state *state, t_parser *parser, t_deque_tt *tokens);
bool		is_simple_cmd_token(t_tt tt);
t_ast_node	parse_compound_list(t_state *state, t_parser *parser,
				t_deque_tt *tokens);
t_ast_node	parse_subshell(t_state *state, t_parser *parser,
				t_deque_tt *tokens);

// tree_utils.c
void		print_node(t_ast_node node);
void		print_ast_dot(t_state *state, t_ast_node node);
void		ast_postorder_traversal(t_ast_node *node,
				void (*f)(t_ast_node *node));
void		free_ast(t_ast_node *node);

char		*tt_to_str(t_tt tt);
void		free_ast(t_ast_node *node);

void		print_tokens(t_deque_tt tokens);

// reparser.c
void		reparse_dquote(t_ast_node *ret, int *i, t_token t);
void		reparse_assignment_words(t_ast_node *node);
void		reparse_envvar(t_ast_node *ret, int *i, t_token t, t_tt tt);
void		reparse_words(t_ast_node *node);
t_ast_node	reparse_word(t_token t);
//[a-zA-Z_]
bool		is_var_name_p1(char c);
//[a-zA-Z0-9_]
bool		is_var_name_p2(char c);
t_ast_node	create_subtoken_node(t_token t,
				int offset, int end_offset, t_tt tt);

// heredoc.c
typedef struct s_heredoc_req
{
	t_string	full_file;
	bool		finished;
	char		*sep;
	bool		expand;
	bool		remove_tabs;
}	t_heredoc_req;

int			gather_heredocs(t_state *state, t_ast_node *node);
bool		contains_quotes(t_ast_node node);
void		write_heredoc(t_state *state, int wr_fd, t_heredoc_req *req);
int			ft_mktemp(t_state *state, t_ast_node *node);
char		*first_non_tab(char *line);
void		expand_line(t_state *state, t_string *full_file, char *line);
bool		is_special_char(char c);
bool		is_space(char c);

// expander
typedef struct s_expander_simple_cmd
{
	bool		found_first;
	bool		export;
	int			exit_stat;
	t_ast_node	*curr;
	size_t		i;
}	t_expander_simple_cmd;
t_env		assignment_to_env(t_state *state, t_ast_node *node);
void		assignment_word_to_word(t_ast_node *node);
t_token_old	get_old_token(t_ast_node word);

char		*expand_word_single(t_state *state, t_ast_node *curr);
void		expand_env_vars(t_state *state, t_ast_node *node);
void		expand_tilde_word(t_state *state, t_ast_node *curr);
// builtins
int			(*builtin_func(char *name))(t_state *state, t_vec argv);

// executor
typedef struct executable_node_s
{
	int			infd;
	int			outfd;
	int			next_infd;
	t_ast_node	*node;
	t_vec_int	redirs;
	bool		modify_parent_context;
}	t_executable_node;

void		execute_top_level(t_state *state);
t_exe_res	execute_pipeline(t_state *state, t_executable_node *exe);

void		set_up_redirection(t_state *state, t_executable_node *exe);
t_exe_res	execute_simple_command(t_state *state, t_executable_node *exe);
void		forward_exit_status(t_exe_res res);
t_exe_res	execute_command(t_state *state, t_executable_node *exe);
t_exe_res	execute_tree_node(t_state *state, t_executable_node *exe);
t_exe_res	execute_simple_list(t_state *state, t_executable_node *exe);
t_string	word_to_string(t_ast_node node);
t_string	word_to_hrdoc_string(t_ast_node node);
void		set_cmd_status(t_state *state, t_exe_res res);

// error.c
void		critical_error(char *error);
void		critical_error_errno(void);
void		critical_error_errno_context(char *context);
void		warning_error(char *error);
void		warning_error_errno(void);
void		err_1_errno(t_state *state, char *p1);
void		err_2(t_state *state, char *p1, char *p2);
t_ast_node	unexpected(t_state *state, t_parser *parser,
				t_ast_node ret, t_deque_tt *tokens);

// utils.c
void		free_tab(char **tab);
// 0 on success
int			write_to_file(char *str, int fd);
void		vec_append_fd(int fd, t_string *ret);

// glob.c
t_vec	expand_word_glob(t_ast_node word);
typedef struct s_dir_matcher
{
	DIR				*dir;
	t_vec		*args;
	t_vec_glob		glob;
	char			*path;
	size_t			offset;
}	t_dir_matcher;
void		match_dir(t_vec *args,
				t_vec_glob glob, char *path, size_t offset);
size_t		matches_pattern(char *name,
				t_vec_glob patt, size_t offset, bool first);
t_vec_glob	word_to_glob(t_ast_node word);
size_t		matches_pattern(char *name, t_vec_glob patt,
				size_t offset, bool first);
void		ft_quicksort(t_vec *vec);

// signals.c
void		set_unwind_sig(void);
void		default_signal_handlers(void);
void		readline_bg_signals(void);
void		set_unwind_sig_norestart(void);

// expanding
void		expand_word(t_state *state, t_ast_node *node,
				t_vec *args, bool keep_as_one);
int			expand_simple_command(t_state *state, t_ast_node *node,
				t_executable_cmd *ret, t_vec_int *redirects);
int			redirect_from_ast_redir(t_state *state,
				t_ast_node *curr, int *redir_idx);
t_vec_nd	split_words(t_state *state, t_ast_node *node);

// hist
void		manage_history(t_state *state);
void		init_history(t_state *state);
void		free_hist(t_state *state);
void		parse_history_file(t_state *state);
t_string	encode_cmd_hist(char *cmd);

// free utils
void		free_all_state(t_state *state);
void		free_redirects(t_vec_redir *v);
void		free_executable_node(t_executable_node *node);
void		free_executable_cmd(t_executable_cmd cmd);

// prompt
t_string	prompt_normal(t_state *state);
t_string	prompt_more_input(t_parser *parser);

// init
void		init_setup(t_state *state, char **argv, char **envp);

// hack
char		*getpid_hack(void);

// execute_input
void		parse_and_execute_input(t_state *state);

// cmd_path
int			find_cmd_path(t_state *state, char *cmd_name, char **path_of_exe);

// get more tokens
void		get_more_tokens(t_state *state, char **prompt, t_deque_tt *tt);

t_exe_res	res_status(int status);
t_exe_res	res_pid(int pid);
void		exe_res_set_status(t_exe_res *res);

# include "env/env.h"

#endif
