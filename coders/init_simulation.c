/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_simulation.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/08 17:37:35 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 17:38:52 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_mutex_and_cond(t_coder *coder, t_sim *sim)
{
	if (pthread_mutex_init(&coder->coder_mutex, NULL))
		return (1);
	sim->coders_init_num++;
	if (pthread_cond_init(&coder->queue_cond, NULL))
		return (1);
	sim->coders_init_num++;
	if (pthread_cond_init(&coder->sleep_cond, NULL))
		return (1);
	sim->coders_init_num++;
	return (0);
}

static int	init_each_coder(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->num_coders)
	{
		if (init_mutex_and_cond(&sim->coders[i], sim))
			return (1);
		sim->dongles[i].id = i;
		sim->dongles[i].available_at = 0;
		sim->coders[i].id = i + 1;
		sim->coders[i].compiles_done = 0;
		sim->coders[i].sim = sim;
		sim->coders[i].owns_hardware = 0;
		sim->coders[i].last_compile_start = 0;
		sim->coders[i].left_dongle = sim->dongles + i;
		sim->coders[i].right_dongle = sim->dongles
			+ ((i + 1) % sim->num_coders);
		sim->coders[i].node.coder = &sim->coders[i];
		i++;
	}
	return (0);
}

static int	init_and_increment(pthread_mutex_t *mutex, pthread_cond_t *cond,
	int *counter)
{
	if (mutex && pthread_mutex_init(mutex, NULL))
		return (1);
	if (mutex)
		(*counter)++;
	if (cond && pthread_cond_init(cond, NULL))
		return (1);
	if (cond)
		(*counter)++;
	return (0);
}

int	init_simulation(t_sim *sim)
{
	sim->dongles = ft_calloc(sim->num_coders, sizeof(t_dongle));
	sim->coders = ft_calloc(sim->num_coders, sizeof(t_coder));
	sim->sleep_heap = init_heap(sim->num_coders, 1337);
	if (!sim->dongles || !sim->coders || !sim->sleep_heap)
		return (1);
	if (init_and_increment(&sim->write_mutex, &sim->start_cond,
			&sim->main_init_step)
		|| init_and_increment(&sim->queue_mutex, &sim->waiter_cond,
			&sim->main_init_step)
		|| init_and_increment(&sim->sleep_mutex, &sim->sleep_room_cond,
			&sim->main_init_step)
		|| init_and_increment(&sim->state_mutex, NULL,
			&sim->main_init_step))
		return (1);
	return (init_each_coder(sim));
}
