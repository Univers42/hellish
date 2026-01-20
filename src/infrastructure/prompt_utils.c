/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 16:34:45 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

t_string prompt_normal(t_shell *state)
{
    t_string ret;
    char *user;
    char cwd[PATH_MAX + 1];
    char *short_cwd;
    int cols;
    char *branch;
    int branch_dirty;
    char *venv;
    char time_buf[32];
    int left_width;
    int time_width;
    int pad;

    (void)state;
    ensure_locale();
    vec_init(&ret);
    ret.elem_size = 1;
    user = getenv("USER");
    if (!user)
        user = "user";
    if (!getcwd(cwd, sizeof(cwd)))
        ft_strcpy(cwd, "~");
    cols = get_cols();
    short_cwd = shorten_path(cwd, ft_max(20, cols - 50));
    branch = NULL;
    branch_dirty = 0;
    get_git_info(&branch, &branch_dirty);
    venv = get_venv_name();
    get_timebuf(time_buf, sizeof(time_buf));
    left_width = 0;
    left_width += measure_width("╭─");
    left_width += 1;
    left_width += ft_strlen(user);
    left_width += 1;
    left_width += measure_width("─");
    left_width += 1;
    left_width += ft_strlen(short_cwd);
    if (branch)
    {
        left_width += 1;
        left_width += measure_width("─");
        left_width += 1;
        left_width += ft_strlen(branch);
    }
    if (venv)
    {
        left_width += 2;
        left_width += ft_strlen(venv);
        left_width += 1;
    }
    vec_push_ansi(&ret, "\033[90m");
    vec_push_str(&ret, "╭─");
    vec_push_ansi(&ret, "\033[0m");
    vec_push_str(&ret, " ");
    vec_push_ansi(&ret, "\033[38;5;208m");
    vec_push_str(&ret, user);
    vec_push_ansi(&ret, "\033[0m");
    vec_push_str(&ret, " ");
    vec_push_ansi(&ret, "\033[90m");
    vec_push_str(&ret, "─");
    vec_push_ansi(&ret, "\033[0m");
    vec_push_str(&ret, " ");
    vec_push_ansi(&ret, "\033[36m");
    vec_push_str(&ret, short_cwd);
    vec_push_ansi(&ret, "\033[0m");
    if (branch)
    {
        vec_push_str(&ret, " ");
        vec_push_ansi(&ret, "\033[90m");
        vec_push_str(&ret, "─");
        vec_push_ansi(&ret, "\033[0m");
        vec_push_str(&ret, " ");
        
        if (branch_dirty)
            vec_push_ansi(&ret, "\033[35m");
        else
            vec_push_ansi(&ret, "\033[37m");
        vec_push_str(&ret, branch);
        vec_push_ansi(&ret, "\033[0m");
    }
    if (venv)
    {
        vec_push_str(&ret, " ");
        vec_push_ansi(&ret, "\033[34m");
        vec_push_str(&ret, "(");
        vec_push_str(&ret, venv);
        vec_push_str(&ret, ")");
        vec_push_ansi(&ret, "\033[0m");
    }
    time_width = measure_width("⏳") + ft_strlen(time_buf);
    pad = cols - left_width - time_width;
    if (pad < 1)
        pad = 1;
    while (pad-- > 0)
    {
        char sp = ' ';
        vec_push(&ret, &sp);
    }
    vec_push_ansi(&ret, "\033[2m");
    vec_push_str(&ret, "⏳");
    vec_push_str(&ret, time_buf);
    vec_push_ansi(&ret, "\033[0m");
    vec_push_str(&ret, "\n");
    vec_push_str(&ret, "$ ");
    free(short_cwd);
    if (branch)
        free(branch);
    if (venv)
        free(venv);
    
    return ret;
}