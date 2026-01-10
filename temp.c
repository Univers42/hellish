# include "history.h"
# include "prompt.h"   // needed for full t_rl definition
# include "exit.h"     // provides warning_error / error helpers
# include <readline/history.h>
# include <fcntl.h>
# include <errno.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

void bg_readline(int outfd, char *prompt)
{
	char *ret;

	rl_instream = stdin;
	rl_outstream = stderr;
	ret = readline(prompt);
	if (!ret)
	{
		close(outfd);
		exit(1);
	}
	write_to_file(ret, outfd);
	close(outfd);
	exit(0);
}

int attach_input_readline(t_rl *l, int pp[2], int pid)
{
	int status;

	close(pp[1]);
	dyn_str_append_fd(&l->str, pp[0]);
	buff_readline_update(l);
	close(pp[0]);
	while (1)
		if (waitpid(pid, &status, 0) != -1)
			break;
	if (WIFSIGNALED(status))
	{
		ft_eprintf("\n");
		return (2);
	}
	return (WEXITSTATUS(status));
}

static void	readline_bg_signals(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_IGN);
}

int get_more_input_readline(t_rl *l, char *prompt)
{
	int pp[2];
	int pid;
	struct sigaction old_sigint, old_sigquit, ign;
	t_repl_config *rc;

	rc = get_repl_config();
	/* If REPL handles signals, protect parent from SIGINT while child runs */
	if (rc == NULL || rc->handle_signals)
	{
		ft_memset(&ign, 0, sizeof(ign));
		ign.sa_handler = SIG_IGN;
		sigemptyset(&ign.sa_mask);
		sigaction(SIGINT, &ign, &old_sigint);
		sigaction(SIGQUIT, &ign, &old_sigquit);
	}

	if (pipe(pp))
		critical_error_errno_context("pipe");
	pid = fork();
	if (pid == 0)
	{
		/* child restores desired handlers for readline */
		readline_bg_signals();
		close(pp[0]);
		bg_readline(pp[1], prompt);
	}
	else if (pid < 0)
		critical_error_errno_context("fork");
	else
	{
		int ret = attach_input_readline(l, pp, pid);
		/* restore parent's original signal handlers if we changed them */
		if (rc == NULL || rc->handle_signals)
		{
			sigaction(SIGINT, &old_sigint, NULL);
			sigaction(SIGQUIT, &old_sigquit, NULL);
		}
		return (ret);
	}
	ft_assert("Unreachable" == 0);
	return (0);
}

void buff_readline_update(t_rl *l)
{
	l->has_line = l->cursor != l->str.len;
}

void buff_readline_reset(t_rl *l)
{
	ft_memmove(l->str.buff, l->str.buff + l->cursor, l->str.len - l->cursor);
	l->str.len -= l->cursor;
	if (l->str.buff)
		l->str.buff[l->str.len] = 0;
	l->cursor = 0;
	buff_readline_update(l);
}

void buff_readline_init(t_rl *ret)
{
	*ret = (t_rl){0};
}

void update_context(t_rl *rl, char **context, char **base_context)
{
	char *new_ctx;
	int len;

	if (!rl->should_update_ctx)
		return;
	len = ft_strlen(*base_context) + 16;
	new_ctx = (char *)malloc((size_t)len);
	if (!new_ctx)
		return;
	snprintf(new_ctx, (size_t)len, "%s: line %d", *base_context, rl->lineno);
	if (*context)
		free(*context);
	*context = new_ctx;
}

int get_more_input_notty(t_rl *rl)
{
	char buff[4096 * 2];
	int ret;
	int status;

	status = 1;
	/* Respect REPL configuration for signal handling */
	{
		t_repl_config *rc = get_repl_config();
		if (rc == NULL || rc->handle_signals)
			set_unwind_sig_norestart();
	}
	while (1)
	{
		ret = read(0, buff, sizeof(buff));
		if (ret < 0 && errno == EINTR)
			status = 2;
		if (ret == 0)
			rl->has_finished = true;
		if (ret == 0)
			dyn_str_pushstr(&rl->str, "\n");
		if (ret <= 0)
			break;
		status = 0;
		dyn_str_pushnstr(&rl->str, buff, ret);
		if (ft_strnchr(buff, '\n', ret))
			break;
	}
	/* Restore unwind handlers only if we installed them */
	{
		t_repl_config *rc = get_repl_config();
		if (rc == NULL || rc->handle_signals)
			set_unwind_sig();
	}
	buff_readline_update(rl);
	return (status);
}

static int readline_cmd(t_rl *rl, t_getmore_ctx *ctx)
{
	int stat;
	t_xreadline_ctx xctx;

	/* build small xreadline context from getmore context */
	xctx.ret = ctx->input;
	xctx.prompt = *ctx->prompt;
	xctx.last_cmd_status_res = ctx->last_cmd_status_res;
	xctx.last_cmd_status_s = ctx->last_cmd_status_s;
	xctx.input_method = ctx->input_method;
	xctx.context = ctx->context;
	xctx.base_context = ctx->base_context;

	stat = xreadline(rl, &xctx);
	if (stat == 0)
		return (1);
	if (stat == 2)
	{
		if (*ctx->input_method != INP_READLINE)
			*ctx->should_exit = true;
		return (2);
	}
	return (0);
}

bool ends_with_bs_nl(t_dyn_str s)
{
	size_t i;
	bool unterminated;

	if (s.len == 0)
		return (false);
	i = s.len;
	unterminated = false;
	if (s.buff[--i] != '\n')
		return (false);
	while (i > 0)
	{
		i--;
		if (s.buff[i] == '\\')
			unterminated = !unterminated;
		else
			break;
	}
	return (unterminated);
}

static void extend_bs(t_rl *rl, t_getmore_ctx *ctx)
{
	char *tmp_prompt;
	char *old_prompt;

	while (ends_with_bs_nl(*ctx->input))
	{
		dyn_str_pop(ctx->input);
		dyn_str_pop(ctx->input);
		tmp_prompt = ft_strdup("> ");
		if (!tmp_prompt)
			return;
		/* swap in the temporary prompt for the duration of the call */
		old_prompt = *ctx->prompt;
		*ctx->prompt = tmp_prompt;
		if (readline_cmd(rl, ctx))
		{
			/* restore and free temp before returning */
			*ctx->prompt = old_prompt;
			free(tmp_prompt);
			return;
		}
		/* restore and free temp */
		*ctx->prompt = old_prompt;
		free(tmp_prompt);
	}
}

/* sanitize input buffer: replace invalid UTF-8 leading/continuation with '?' so
   downstream mbrtowc/vis_width/tokenizer do not loop on invalid bytes */
static void sanitize_input_utf8(t_dyn_str *input)
{
	mbstate_t st;
	wchar_t wc;
	size_t r;
	size_t i = 0;
	ft_memset(&st, 0, sizeof(st));
	while (i < input->len)
	{
		r = ft_mbrtowc(&wc, input->buff + i, (size_t)(input->len - i), &st);
		if (r == (size_t)-1)
		{
			/* invalid sequence: replace single byte and continue */
			input->buff[i] = '?';
			ft_memset(&st, 0, sizeof(st));
			i++;
			continue;
		}
		if (r == (size_t)-2)
		{
			/* truncated sequence at end: leave as-is */
			break;
		}
		if (r == 0)
			break;
		i += r;
	}
}

void get_more_tokens(t_rl *rl, t_getmore_ctx *ctx)
{
	int stat;
	char *curr_prompt;
	char *next_prompt;
	t_deque tt;
	t_deque *tokens = NULL;
	bool created_local = false;
	char looking_for = '\0';

	/* Only create a temporary deque if caller did not provide one. */
	if (!ctx->out_tokens)
	{
		deque_init(&tt, 64, sizeof(t_token), NULL);
		tokens = &tt;
		created_local = true;
	}
	else
		tokens = ctx->out_tokens;

	while (ctx->prompt && *ctx->prompt)
	{
		curr_prompt = *ctx->prompt;
		stat = readline_cmd(rl, ctx);
		if (stat == 1)
		{
			if (looking_for && ctx->input->len)
				ft_eprintf("%s: unexpected EOF while looking for matching `%c'\n",
						   (ctx->context && *ctx->context) ? *ctx->context : "input", looking_for);
			else if (ctx->input->len)
				ft_eprintf("%s: syntax error: unexpected end of file\n", (ctx->context && *ctx->context) ? *ctx->context : "input");
			if (*ctx->input_method == INP_READLINE)
				ft_eprintf("exit\n");
			if (!ctx->last_cmd_status_res->status && ctx->input->len)
				set_cmd_status(ctx->last_cmd_status_res, (t_status){.status = SYNTAX_ERR}, ctx->last_cmd_status_s);
			*ctx->should_exit = true;
		}
		if (stat)
		{
			free(curr_prompt);
			if (created_local && tokens && tokens->buf)
				free(tokens->buf);
			return;
		}
		extend_bs(rl, ctx);
		/* sanitize input so invalid UTF-8 bytes don't cause downstream loops */
		sanitize_input_utf8(ctx->input);
		/* Tokenize into the (possibly caller-provided) deque */
		next_prompt = tokenizer(ctx->input->buff, tokens);
		free(curr_prompt);
		if (next_prompt)
			*ctx->prompt = ft_strdup(next_prompt);
		else
			*ctx->prompt = NULL;
	}
	/* Free local deque buffer if we created it here */
	if (created_local && tokens && tokens->buf)
		free(tokens->buf);
}

void free_tab(char **tab)
{
	int i;

	i = 0;
	while (tab[i])
		free(tab[i++]);
	free(tab);
}

// 0 on success
int	write_to_file(char *str, int fd)
{
	int	wrote_total;
	int	wrote;
	int	len;

	len = ft_strlen(str);
	wrote_total = 0;
	while (wrote_total != len)
	{
		wrote = write(fd, str + wrote_total, len - wrote_total);
		if (wrote < 0)
			return (1);
		wrote_total += wrote;
	}
	return (0);
}

static void	default_signal_handlers(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

void forward_exit_status(t_status res)
{
	ft_assert(res.status != -1);

	/* Free small REPL-owned allocations (avoid "still reachable" after valgrind) */
	{
		t_api_readline *state = get_repl_state_ptr();
		if (state)
		{
			if (state->last_cmd_status_s)
			{
				free(state->last_cmd_status_s);
				state->last_cmd_status_s = NULL;
			}
			/* do not free entire state here — main cleanup runs elsewhere on normal exit */
		}
	}

	if (res.c_c)
	{
		default_signal_handlers();
		kill(0, SIGINT);
		while (true)
			;
	}
	exit(res.status);
}

/**
 * We need the function to update teh caller's char* pointer (state->last_cmd_status_s).
 * in C alll arguments are passed by value, so passing char* gives teh function a copy of the
 * pointer. Freeing via that copy will free the same heap block but assigning a new
 * malloc/ft_itoa result tht local copy does not update the caller's pointer.
 * Teh new pointer is lost and leaks
 *
 * PINPOINT: change to char *** when teh function must change the caller's pointer itserlf otherwise
 * return new pointer and let the caller assign it
 */
void set_cmd_status(t_status *last_cmd_status_res, t_status res, char **last_cmd_status_s)
{
	*last_cmd_status_res = res;
	if (*last_cmd_status_s)
		free(*last_cmd_status_s);
	*last_cmd_status_s = ft_itoa(res.status);
}

t_dyn_str parse_single_cmd(t_dyn_str hist, size_t *cur)
{
	t_dyn_str cmd;
	bool bs;
	char c;

	dyn_str_init(&cmd);
	bs = false;
	while (*cur < hist.len)
	{
		c = hist.buff[*cur];
		if (c == '\\' && !bs)
		{
			bs = true;
			(*cur)++;
			continue;
		}
		if (c == '\n' && !bs)
		{
			(*cur)++;
			break;
		}
		dyn_str_push(&cmd, c);
		bs = false;
		(*cur)++;
	}
	return (cmd);
}

t_vec parse_hist_file(t_dyn_str hist)
{
	size_t cur;
	t_vec ret;
	char *cmd;
	char *cmd_ptr;

	cur = 0;
	vec_init(&ret, &(t_vec_config){
					   .elem_size = sizeof(char *),
					   .initial_capacity = 64,
					   .type_mask = VEC_TYPE_PTR});
	while (cur < hist.len)
	{
		cmd = parse_single_cmd(hist, &cur).buff;
		cmd_ptr = cmd;
		vec_push(&ret, &cmd_ptr);
		add_history(cmd);
	}
	return (ret);
}

char *get_hist_file_path(t_vec *_env)
{
	t_env *env;
	t_dyn_str full_path;

	env = env_get(_env, "HOME");
	if (!env || !env->value)
	{
		warning_error("HOME is not set, can't get the history");
		return (0);
	}
	dyn_str_init(&full_path);
	dyn_str_pushstr(&full_path, env->value);
	if (!dyn_str_ends_with_str(&full_path, "/"))
		dyn_str_push(&full_path, '/');
	dyn_str_pushstr(&full_path, HIST_FILE);
	return (full_path.buff);
}

void parse_history_file(t_hist *_hist, t_vec *env)
{
	t_dyn_str hist;
	int fd;
	char *hist_file_path;

	hist_file_path = get_hist_file_path(env);
	if (!hist_file_path)
		return ;
	fd = open(hist_file_path, O_RDONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		warning_error("Can't open the history file for reading");
		free(hist_file_path);
		return;
	}
	dyn_str_init(&hist);
	dyn_str_append_fd(&hist, fd);
	close(fd);
	_hist->cmds = parse_hist_file(hist);
	_hist->append_fd = open(hist_file_path, O_CREAT | O_WRONLY | O_APPEND, 0666);
	if (_hist->append_fd < 0)
		warning_error("Can't open the history file for writing");
	free(hist_file_path);
	free(hist.buff);
}

t_dyn_str encode_cmd_hist(char *cmd)
{
	t_dyn_str ret;

	dyn_str_init(&ret);
	while (*cmd)
	{
		if (*cmd == '\\')
			dyn_str_push(&ret, '\\');
		if (*cmd == '\n')
			dyn_str_push(&ret, '\\');
		dyn_str_push(&ret, *cmd);
		cmd++;
	}
	dyn_str_push(&ret, '\n');
	return (ret);
}

bool worthy_of_being_remembered(t_hist *hist, t_rl *rl)
{
	// Determine actual entered command length:
	// - prefer rl->cursor (cursor points after last typed char),
	// - fallback to entire buffer length if cursor is not set.
	size_t entry_len = 0;
	if (rl->cursor > 0)
	{
		// rl->cursor is count of characters including a trailing newline pushed by xreadline
		// historically callers used rl->cursor - 1; guard against underflow
		if (rl->cursor > 0)
			entry_len = rl->cursor - 1;
	}
	if (entry_len == 0)
		entry_len = rl->str.len;

	// Only remember non-empty entries and when history is active.
	if (entry_len > 0 && hist->active &&
		( !hist->cmds.len ||
		  !str_slice_eq_str(rl->str.buff, entry_len,
							*(char **)vec_idx(&hist->cmds, hist->cmds.len - 1)) ))
	{
		return (true);
	}
	return (false);
}

void manage_history(t_hist *hist, t_rl *rl)
{
	t_repl_config *conf = get_repl_config();

	if (conf && !conf->enable_history)
		return ;
	char *hist_entry;
	char *enc_hist_entry;

	/* compute entry length the same way as worthy_of_being_remembered */
	size_t entry_len = 0;
	if (rl->cursor > 0)
	{
		/* guard: rl->cursor is count including trailing newline pushed by xreadline */
		if (rl->cursor > 0)
			entry_len = rl->cursor - 1;
	}
	if (entry_len == 0)
		entry_len = rl->str.len;

	if (entry_len == 0)
		return;

	if (worthy_of_being_remembered(hist, rl))
	{
		hist_entry = ft_strndup(rl->str.buff, entry_len);
		add_history(hist_entry);
		vec_push(&hist->cmds, &hist_entry);
		if (hist->append_fd >= 0)
		{
			enc_hist_entry = encode_cmd_hist(hist_entry).buff;
			if (write_to_file(enc_hist_entry, hist->append_fd) < 0)
			{
				warning_error("Failed to write to the history file");
				close(hist->append_fd);
				hist->append_fd = -1;
			}
			free(enc_hist_entry);
		}
	}
}

void manage_history_input(t_hist *hist, t_rl *rl, t_dyn_str *input)
{
	t_repl_config *conf = get_repl_config();
	char *hist_entry;
	char *enc_hist_entry;
	size_t entry_len;

	(void)rl;
	if (conf && !conf->enable_history)
		return ;
	if (!input || input->len == 0)
		return ;
	entry_len = input->len;
	/* trim trailing newline if present */
	if (entry_len > 0 && input->buff[entry_len - 1] == '\n')
		entry_len--;
	if (entry_len == 0)
		return ;
	/* skip duplicate of last history entry */
	if (hist->cmds.len)
	{
		char *last = *(char **)vec_idx(&hist->cmds, hist->cmds.len - 1);
		if (last && str_slice_eq_str(input->buff, entry_len, last))
			return;
	}
	hist_entry = ft_strndup(input->buff, entry_len);
	add_history(hist_entry);
	vec_push(&hist->cmds, &hist_entry);
	if (hist->append_fd >= 0)
	{
		enc_hist_entry = encode_cmd_hist(hist_entry).buff;
		if (write_to_file(enc_hist_entry, hist->append_fd) < 0)
		{
			warning_error("Failed to write to the history file");
			close(hist->append_fd);
			hist->append_fd = -1;
		}
		free(enc_hist_entry);
	}
}

void init_history(t_hist *hist, t_vec *env)
{
	*hist = (t_hist){.append_fd = -1, .active = true};
	parse_history_file(hist, env);
}

void free_hist(t_hist *hist)
{
	size_t i;

	i = 0;
	while (i < hist->cmds.len)
	{
		free(*(char **)vec_idx(&hist->cmds, i));
		i++;
	}
	free(hist->cmds.buff);
	vec_init(&hist->cmds, &(t_vec_config){0});
}

t_dyn_str prompt_normal(t_status *st_res, char **st_s)
{
	t_dyn_str p;
	t_dyn_str cwd;
	char *user;
	char *short_path;
	char time_buf[32];
	int cols;
	int status;
	int line_w;
	int chrono_w;
	int pad;

	static char *cached_git_branch = NULL;
	static int cached_git_dirty = 0;
	static char *cached_cwd = NULL;
	char *curr_cwd;
	int need_refresh = 0;
	t_repl_config *conf = get_repl_config();

	(void)st_s;
	dyn_str_init(&p);
	ensure_locale();
	user = getenv("USER");
	if (!user)
		user = "inferno";
	cap_cmd("pwd", &cwd);
	curr_cwd = cwd.buff;

	// Only refresh git info if cwd changed AND vcs enabled
	if ((conf == NULL || conf->enable_vcs) && (cached_cwd == NULL || strcmp(cached_cwd, curr_cwd) != 0))
		need_refresh = 1;
	if (need_refresh && (conf == NULL || conf->enable_vcs))
	{
		t_vcs_info git = get_git();
		if (cached_git_branch)
			free(cached_git_branch);
		cached_git_branch = git.data ? ft_strdup(git.data) : NULL;
		cached_git_dirty = git.dirty;
		if (cached_cwd)
			free(cached_cwd);
		cached_cwd = ft_strdup(curr_cwd);
		if (git.data)
			free(git.data);
	}
	cols = get_cols();
	status = st_res->status;
	short_path = shorten_path(cwd.buff, cols - 60);

	/* ══════════════════════════════════════════════════════════════════════
	   LINE 1: ╭─  user ═══  path ║  branch ●
	   ══════════════════════════════════════════════════════════════════════ */

	/* Opening decoration */
	dyn_str_pushstr(&p, FG_CHARCOAL);
	dyn_str_pushstr(&p, "╭─");
	dyn_str_pushstr(&p, RESET);

	/* User segment */
	dyn_str_pushstr(&p, " ");
	dyn_str_pushstr(&p, FG_MOLTEN);
	dyn_str_pushstr(&p, USER_ICON);
	dyn_str_pushstr(&p, " ");
	dyn_str_pushstr(&p, BOLD);
	dyn_str_pushstr(&p, user);
	dyn_str_pushstr(&p, RESET);

	/* Separator */
	dyn_str_pushstr(&p, FG_CHARCOAL);
	dyn_str_pushstr(&p, SEP_L);
	dyn_str_pushstr(&p, RESET);

	/* Path segment */
	dyn_str_pushstr(&p, FG_EMBER);
	dyn_str_pushstr(&p, DIR_ICON);
	dyn_str_pushstr(&p, " ");
	dyn_str_pushstr(&p, short_path);
	dyn_str_pushstr(&p, RESET);

	/* Git segment */
	if (cached_git_branch)
	{
		dyn_str_pushstr(&p, FG_CHARCOAL);
		dyn_str_pushstr(&p, SEP_GIT);
		dyn_str_pushstr(&p, RESET);
		if (cached_git_dirty)
			dyn_str_pushstr(&p, FG_PURPLE);
		else
			dyn_str_pushstr(&p, FG_ASH);
		dyn_str_pushstr(&p, GIT_ICON);
		dyn_str_pushstr(&p, " ");
		dyn_str_pushstr(&p, cached_git_branch);
		dyn_str_pushstr(&p, RESET);
	}

	/* Status code if error */
	if (status != 0)
	{
		dyn_str_pushstr(&p, " ");
		dyn_str_pushstr(&p, FG_FIRE);
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, "⟦");
		dyn_str_pushstr(&p, RESET);
		dyn_str_pushstr(&p, FG_WARN);
		char stat_buf[16];
		snprintf(stat_buf, sizeof(stat_buf), "%d", status);
		dyn_str_pushstr(&p, stat_buf);
		dyn_str_pushstr(&p, RESET);
		dyn_str_pushstr(&p, FG_FIRE);
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, "⟧");
		dyn_str_pushstr(&p, RESET);
	}

	/* ══════════════════════════════════════════════════════════════════════
	   RIGHT-ALIGNED CHRONO (only if enabled)
	   ══════════════════════════════════════════════════════════════════════ */
	if (conf == NULL || conf->enable_chrono)
	{
		fmt_time(time_buf, sizeof(time_buf), get_chrono()->last_ms);
		line_w = vis_width(p.buff);
		chrono_w = 3 + (int)ft_strlen(time_buf); /* "⏱ " + time */
		pad = cols - line_w - chrono_w - 1;
		if (pad < 2)
			pad = 2;
		while (pad-- > 0)
			dyn_str_push(&p, ' ');
		if (get_chrono()->last_ms >= 100)
			dyn_str_pushstr(&p, FG_WARN);
		else
			dyn_str_pushstr(&p, FG_DIM);
		dyn_str_pushstr(&p, TIMER_ICON);
		dyn_str_pushstr(&p, " ");
		dyn_str_pushstr(&p, time_buf);
		dyn_str_pushstr(&p, RESET);
	}
	// else: skip chrono entirely (no padding added)

	/* ══════════════════════════════════════════════════════════════════════
	   LINE 2: status circle (color by ultimate state)
	   ══════════════════════════════════════════════════════════════════════ */
	dyn_str_pushstr(&p, "\n");
	dyn_str_pushstr(&p, FG_CHARCOAL);
	dyn_str_pushstr(&p, "╰─");
	dyn_str_pushstr(&p, RESET);

	// Status color logic - priority: signal > exit_code > success
	if (st_res->c_c)
	{
		dyn_str_pushstr(&p, FG_PURPLE); // signal (Ctrl+C, etc.)
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, " ●");
	}
	else if (status == 0)
	{
		dyn_str_pushstr(&p, FG_SUCCESS); // green = success
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, " ●");
	}
	else if (status == 1)
	{
		dyn_str_pushstr(&p, FG_BLUE); // blue = generic error
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, " ●");
	}
	else if (status >= 128)
	{
		dyn_str_pushstr(&p, FG_WARN); // yellow = signal termination
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, " ●");
	}
	else
	{
		dyn_str_pushstr(&p, FG_FIRE); // red = other error
		dyn_str_pushstr(&p, BOLD);
		dyn_str_pushstr(&p, " ●");
	}
	dyn_str_pushstr(&p, RESET);
	dyn_str_pushstr(&p, " ");

	free(cwd.buff);
	free(short_path);
	return (p);
}

/* ═══════════════════════════════════════════════════════════════════════════
   MORE INPUT PROMPT
   ═══════════════════════════════════════════════════════════════════════════ */

t_dyn_str prompt_more_input(t_parse *parser)
{
	t_dyn_str ret;
	t_token_type curr;
	size_t i;

	dyn_str_init(&ret);
	dyn_str_pushstr(&ret, FG_CHARCOAL);
	i = 0;
	while (i < parser->stack.len)
	{
		curr = (t_token_type)(uintptr_t)vec_idx(&parser->stack, i++);
		if (curr == TOKEN_LEFT_BRACE)
			dyn_str_pushstr(&ret, "subsh");
		else if (curr == TOKEN_PIPE)
			dyn_str_pushstr(&ret, "pipe");
		else if (curr == TOKEN_LOGICAL_AND)
			dyn_str_pushstr(&ret, "and");
		else if (curr == TOKEN_LOGICAL_OR)
			dyn_str_pushstr(&ret, "or");
		else
			continue;
		dyn_str_pushstr(&ret, " ");
	}
	dyn_str_pushstr(&ret, RESET);
	dyn_str_pushstr(&ret, FG_EMBER);
	dyn_str_pushstr(&ret, "..> ");
	dyn_str_pushstr(&ret, RESET);
	return (ret);
}

static int return_last_line(t_rl *rl, t_dyn_str *ret)
{
	int len;

	len = rl->str.len - rl->cursor;
	dyn_str_pushnstr(ret, rl->str.buff + rl->cursor, len);
	rl->cursor = 0;
	rl->str.len = 0;
	rl->has_line = false;
	if (len == 0)
		return (1);
	return (4);
}

static int return_new_line(t_rl *rl, t_dyn_str *ret, char **context, char **base_context)
{
	char *temp;
	int len;

	rl->lineno++;
	update_context(rl, context, base_context);
	temp = ft_strchr(rl->str.buff + rl->cursor, '\n');
	if (temp == 0)
		return (return_last_line(rl, ret));
	len = temp - (rl->str.buff + rl->cursor) + 1;
	if (len)
		dyn_str_pushnstr(ret, rl->str.buff + rl->cursor, len);
	rl->cursor += len;
	rl->has_line = rl->cursor != rl->str.len;
	if (len == 0)
		return (1);
	return (4);
}

int xreadline(t_rl *rl, t_xreadline_ctx *ctx)
{
	int code;

	if (!ctx)
		return (0);
	if (rl->has_finished)
		return (0);
	if (!rl->has_line)
	{
		if (*ctx->input_method == INP_ARG || *ctx->input_method == INP_FILE)
			return (rl->has_finished = true, 0);
		if (*ctx->input_method == INP_STDIN_NOTTY)
			code = get_more_input_notty(rl);
		else
			code = get_more_input_readline(rl, ctx->prompt);
		if (code == 1)
			return (rl->has_finished = true, 0);
		if (code == 2)
		{
			g_should_unwind = SIGINT;
			set_cmd_status(ctx->last_cmd_status_res, (t_status){.status = CANCELED, .c_c = true}, ctx->last_cmd_status_s);
			return (2);
		}
		if (*ctx->input_method == INP_READLINE)
			dyn_str_push(&rl->str, '\n');
		rl->has_line = true;
	}
	return (return_new_line(rl, ctx->ret, ctx->context, ctx->base_context));
}