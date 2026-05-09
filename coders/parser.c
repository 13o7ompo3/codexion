/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:26 by obahya            #+#    #+#             */
/*   Updated: 2026/05/09 16:59:24 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <string.h>

static int	is_valid_number(char *str)
{
	int	j;

	if (!str || !str[str[0] == '+'])
		return (1);
	j = 0;
	if (*str == '+')
		str++;
	while (*str == '0')
		str++;
	if ((strlen(str) > 10)
		|| ((strlen(str) == 10) && *str == '2'
			&& strcmp(str, "2147483647") > 0))
		return (1);
	while (str[j])
	{
		if (str[j] < '0' || str[j] > '9')
			return (1);
		j++;
	}
	return (0);
}

static int	validat_args(int argc, char **argv)
{
	int	i;

	if (argc != 9 || (strcmp(argv[8], "edf") && strcmp(argv[8], "fifo")))
		return (1);
	i = 1;
	while (i < argc - 1)
	{
		if (is_valid_number(argv[i]))
			return (1);
		i++;
	}
	return (0);
}

int	parse_args(t_sim *sim, int argc, char **argv)
{
	if (validat_args(argc, argv))
		return (1);
	sim->num_coders = atoi(argv[1]);
	sim->time_to_burnout = atoi(argv[2]);
	sim->time_to_compile = atoi(argv[3]);
	sim->time_to_debug = atoi(argv[4]);
	sim->time_to_refactor = atoi(argv[5]);
	sim->required_compiles = atoi(argv[6]);
	sim->dongle_cooldown = atoi(argv[7]);
	sim->scheduler_type = 1;
	if (!strcmp(argv[8], "edf"))
		sim->scheduler_type = 0;
	sim->is_active = 1;
	sim->threads_ready = 0;
	sim->queue = NULL;
	sim->main_init_step = 0;
	sim->coders_init_num = 0;
	sim->coders_remaining = sim->num_coders;
	if (init_simulation(sim))
		return (2);
	return (0);
}
