/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shell.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 15:44:35 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 15:44:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../incs/shell.h"

/**
 * This is voluntarely simple and more visible to separate correctly the
 * complexity of the code
 * it's divided in tree
 * first the lod_cache which is basically a customized api function
 * that let the user to initialize any data he wants (maybe useless but
 * still clean)
 * once The datas will be ocrrecly initialized there will be two types of
 * groups's datas. 
 * First) The one that we want to liberate recurrently in a pool, each iteration
 * of the loop process
 * Second), we empty the cache alias persistent datas only once we want to 
 * shudown the program this what do the shudown program..
 * 
 * 
 * The process, finally is an infinte loop that will break on error or
 * volunteer exit from the user from exit command or CTRL + D
 * 
 * For speedness purposes the program use a cache to so we will get some still
 * reachable datas on readline due to the library. 
 * meaning that if not carefully handled the readline members can easily have
 * undefined behavior because we would not cause a segmentation fault 
 * by dereferencing the values inside the slots of memory asked.
 */
int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;

	shell = (t_shell)
	{
		.argc = argc;
		.argv = argv;
		.envp = envp;
	}
	shell.load_cache();
	shell.process();
	shell.shutdown();
}
