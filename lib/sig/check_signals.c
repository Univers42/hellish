/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   check_signals.c                                    :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"

void check_signals(void)
{
	check_read_timeout();
	quit();
}
