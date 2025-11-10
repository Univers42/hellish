/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tparam.c                                           :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "termcap.h"

static void append_char(char **op, char c)
{
    *(*op)++ = c;
}

static void out_dec_width(char **op, int v, int w)
{
    if (w == 3 && v > 999)
        append_char(op, (v / 1000) + '0');
    if (w >= 3)
        append_char(op, (v / 100) % 10 + '0');
    if (w >= 2)
        append_char(op, (v / 10) % 10 + '0');
    append_char(op, (v % 10) + '0');
}

static char *grow(char *base, char **op, int *cap)
{
    int used;
    char *nb;

    used = *op - base;
    *cap *= 2;
    nb = (char *)xrealloc(base, *cap);
    *op = nb + used;
    return (nb);
}

static void op_swap(int *a, int *b)
{
    int t;

    t = *a;
    *a = *b;
    *b = t;
}

static void op_arith(int *a0, const char **fmt, int *args)
{
    int val;
    int op;

    op = (*fmt)[0];
    if ((*fmt)[1] == 'p')
        val = args[(((*fmt)[2]) & 0177) - 0100];
    else
        val = ((*fmt)[2]) & 0177;
    if (op == '-')
        *a0 -= val;
    else if (op == '+')
        *a0 += val;
    else if (op == '*')
        *a0 *= val;
    else if (op == '/')
        *a0 /= (val ? val : 1);
    else
        *a0 = val;
    *fmt += 3;
}

char *tparam(const char *fmt, char *out, int size,
             int a0, int a1, int a2, int a3)
{
    int args[4];
    int *argp;
    char *buf;
    char *op;
    int cap;

    args[0] = a0;
    args[1] = a1;
    args[2] = a2;
    args[3] = a3;
    argp = args;
    if (!fmt)
        return (NULL);
    buf = out ? out : (char *)xmalloc(size);
    op = buf;
    cap = size;
    while (*fmt)
    {
        if (op - buf + 8 >= cap)
            buf = grow(buf, &op, &cap);
        if (*fmt != '%')
        {
            append_char(&op, *fmt++);
            continue;
        }
        fmt++;
        if (*fmt == '%')
        {
            append_char(&op, '%');
            fmt++;
            continue;
        }
        /* Use a snapshot of current arg for output-oriented ops */
        {
            int c = *fmt++;
            int tem = *argp;

            if (c == 'd' || c == '2' || c == '3')
            {
                out_dec_width(&op, tem, (c == 'd') ? 1 : (c - '0'));
                argp++;
            }
            else if (c == '.')
            {
                append_char(&op, (char)tem);
                argp++;
            }
            else if (c == '+')
            {
                append_char(&op, (char)(tem + *fmt++));
                argp++;
            }
            else if (c == 'i')
            {
                args[0]++;
                args[1]++;
            }
            else if (c == 'r')
            {
                op_swap(&args[0], &args[1]);
            }
            else if (c == 'f')
                argp++;
            else if (c == 'b')
                argp--;
            else if (c == '>')
            {
                if (argp[0] > *fmt++)
                    argp[0] += *fmt;
                fmt++;
            }
            else if (c == 'a')
            {
                op_arith(&argp[0], &fmt, args);
            }
            else if (c == 'n')
            {
                args[0] ^= 0140;
                args[1] ^= 0140;
            }
            else if (c == 'm')
            {
                args[0] ^= 0177;
                args[1] ^= 0177;
            }
            else if (c == 'B')
                argp[0] += 6 * (tem / 10);
            else if (c == 'D')
                argp[0] -= 2 * (tem % 16);
            else if (c == 'C')
            {
                if (tem >= 96)
                    append_char(&op, (char)(tem / 96));
                append_char(&op, (char)(tem + *fmt++));
                argp++;
            }
            else
            {
                /* Unknown: literal fallback */
                append_char(&op, '%');
                append_char(&op, (char)c);
            }
        }
    }
    *op = '\0';
    return (buf);
}
