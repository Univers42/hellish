/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   set_bash_trapsig.c                                 :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

void set_bash_trapsig(int sig)
{
	bind_var_to_int("BASH_TRAPSIG", sig, 0);
}
