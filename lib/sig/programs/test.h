/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:49:39 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:54:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TEST_H
#define TEST_H

#include "../trap.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* Colors */
#define C_RST "\033[0m"
#define C_RED "\033[31m"
#define C_GRN "\033[32m"
#define C_CYN "\033[36m"
#define C_YEL "\033[33m"

/* Section banner */
#define SECTION(title)                                 \
    do                                                 \
    {                                                  \
        printf(C_CYN "\n=== %s ===\n" C_RST, (title)); \
    } while (0)

/* PASS/FAIL helpers (single-statement macros) */
#define PASS(msg)                                    \
    do                                               \
    {                                                \
        printf(C_GRN "[PASS] " C_RST "%s\n", (msg)); \
    } while (0)
#define FAIL(msg)                                    \
    do                                               \
    {                                                \
        printf(C_RED "[FAIL] " C_RST "%s\n", (msg)); \
    } while (0)

/* Small check helpers */
#define CHECK_TRUE(cond, msg_ok, msg_ko) \
    do                                   \
    {                                    \
        if (cond)                        \
            PASS(msg_ok);                \
        else                             \
            FAIL(msg_ko);                \
    } while (0)

#define CHECK_EQ_INT(a, b, msg_ok, msg_ko) \
    do                                     \
    {                                      \
        if ((a) == (b))                    \
            PASS(msg_ok);                  \
        else                               \
            FAIL(msg_ko);                  \
    } while (0)

#endif
