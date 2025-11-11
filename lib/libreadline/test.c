/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   teste.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/11 01:28:49 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/11 01:34:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(void)
{
    char *input;

    while (1)
    {
        input = readline("sh42> ");
        if (!input)
            break ;
        if (*input)
            add_history(input);
        if (strcmp(input, "replace") == 0)
        {
            rl_replace_line("this is a replaced line.", 0);
            rl_on_new_line();
            rl_redisplay();
            printf("\n");
        }
        else if (strcmp(input, "clear") == 0)
        {
            rl_clear_history();
            printf("History cleared.\n");
        }
        else if (strcmp(input, "exit") == 0)
        {
            free(input);
            break ;
        }
        else
            printf("You entered: %s\n", input);
        free(input);
    }
    return 0;
}