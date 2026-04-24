/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:26 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 18:54:43 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <string.h>

int	init_simulation(t_sim *sim)
static void	init_each_coder(t_sim *sim)
{
	int	i;

	sim->dongles = calloc(sim->num_coders, sizeof(t_dongle));
	sim->coders = calloc(sim->num_coders, sizeof(t_coder));
	sim->queue = NULL;
	sim->sleep_heap = init_heap(sim->num_coders, 1337);
	if (!sim->dongles || !sim->coders || !sim->sleep_heap)
		return (1);
	pthread_mutex_init(&sim->queue_mutex, NULL);
	pthread_cond_init(&sim->waiter_cond, NULL);
	pthread_mutex_init(&sim->sleep_mutex, NULL);
	pthread_cond_init(&sim->sleep_room_cond, NULL);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_init(&sim->coders[i].coder_mutex, NULL);
		pthread_cond_init(&sim->coders[i].queue_cond, NULL);
		pthread_cond_init(&sim->coders[i].sleep_cond, NULL);
		sim->dongles[i].id = i;
		sim->dongles[i].available_at = 0;
		sim->coders[i].id = i + 1;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].sim = sim;
		sim->coders[i].is_finished = 0;
		sim->coders[i].owns_hardware = 0;
		sim->coders[i].last_compile_start = 0;
		sim->coders[i].deadline = 0;
		sim->coders[i].left_dongle = sim->dongles + i;
		sim->coders[i].right_dongle = sim->dongles + ((i + 1) % sim->num_coders);
		sim->coders[i].right_dongle = sim->dongles
			+ ((i + 1) % sim->num_coders);
		sim->coders[i].node.coder = &sim->coders[i];
		i++;
	}
}

int	init_simulation(t_sim *sim)
{
	int	i;

	sim->dongles = calloc(sim->num_coders, sizeof(t_dongle));
	sim->coders = calloc(sim->num_coders, sizeof(t_coder));
	sim->queue = NULL;
	sim->sleep_heap = init_heap(sim->num_coders, 1337);
	if (!sim->dongles || !sim->coders || !sim->sleep_heap)
		return (1);
	pthread_mutex_init(&sim->queue_mutex, NULL);
	pthread_cond_init(&sim->waiter_cond, NULL);
	pthread_mutex_init(&sim->sleep_mutex, NULL);
	pthread_cond_init(&sim->sleep_room_cond, NULL);
	init_each_coder(sim);
	return (0);
}

static int	is_valid_number(char *str)
{
	int	j;

	if ((strlen(str) > 10 + (str[0] == '+'))
		|| ((strlen(str) == 10 + (str[0] == '+'))
			&& str[(str[0] == '+')] > '2') || (atoi(str) < 0))
		return (1);
	j = 0;
	if (str[j] == '+')
		j++;
	while (str[j])
	{
		if (str[j] < '0' || str[j] > '9')
			return (1);
		j++;
	}
	return (0);
}

int	validat_args(int argc, char **argv)
{
	int	i;
	int	j;

	if (argc != 9 || (strcmp(argv[8], "edf") && strcmp(argv[8], "fifo")))
		return (1);
	i = 1;
	while (i < argc - 1)
	{
		if ((strlen(argv[i]) > 10 + (argv[i][0] == '+')) ||
		((strlen(argv[i]) == 10 + (argv[i][0] == '+'))
			&& argv[i][(argv[i][0] == '+')] > '2') ||
		(atoi(argv[i]) < 0))
		if (is_valid_number(argv[i]))
			return (1);
		j = 0;
		if (argv[i][j] == '+')
			j++;
		while (argv[i][j])
		{
			if (argv[i][j] < '0' || argv[i][j] > '9')
				return (1);
			j++;
		}
		i++;
	}
	return (0);
}

int	parse_args(t_sim *sim, int argc, char **argv)
{
	sim->coders = NULL;
	sim->dongles = NULL;
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
	pthread_cond_init(&sim->start_cond, NULL);
	pthread_mutex_init(&sim->write_mutex, NULL);
	if (init_simulation(sim))
		return (2);
	return (0);
}
