/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 21:09:16 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 17:33:22 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void	pull_the_fire_alarm(t_sim *sim)
{
	int	i;

	pthread_mutex_lock(&sim->state_mutex);
	sim->is_active = 0;
	pthread_mutex_unlock(&sim->state_mutex);
	pthread_mutex_lock(&sim->queue_mutex);
	pthread_mutex_lock(&sim->sleep_mutex);
	pthread_mutex_lock(&sim->write_mutex);
	pthread_cond_broadcast(&sim->waiter_cond);
	pthread_cond_broadcast(&sim->sleep_room_cond);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_lock(&sim->coders[i].coder_mutex);
		sim->coders[i].is_sleeping = 0;
		pthread_cond_broadcast(&sim->coders[i].queue_cond);
		pthread_cond_broadcast(&sim->coders[i].sleep_cond);
		pthread_mutex_unlock(&sim->coders[i].coder_mutex);
		i++;
	}
	pthread_mutex_unlock(&sim->write_mutex);
	pthread_mutex_unlock(&sim->sleep_mutex);
	pthread_mutex_unlock(&sim->queue_mutex);
}

static int	create_threads(t_sim *sim, int *coders_created)
{
	int	i;

	i = 0;
	while (i < sim->num_coders)
	{
		if (pthread_create(&sim->coders[i].thread_id, NULL, &coder_routine,
				&sim->coders[i]) != 0)
			return (1);
		*coders_created = i + 1;
		i++;
	}
	if (pthread_create(&sim->monitor_thread, NULL, &monitor_routine, sim) != 0)
		return (2);
	if (pthread_create(&sim->waiter_thread, NULL, &waiter_routine, sim) != 0)
		return (3);
	if (pthread_create(&sim->timer_thread, NULL, &sleep_room_routine, sim) != 0)
		return (4);
	return (0);
}

static void	join_threads(t_sim *sim)
{
	int	i;

	pthread_join(sim->monitor_thread, NULL);
	pthread_mutex_lock(&sim->queue_mutex);
	pthread_cond_broadcast(&sim->waiter_cond);
	pthread_mutex_unlock(&sim->queue_mutex);
	pthread_join(sim->waiter_thread, NULL);
	pthread_mutex_lock(&sim->sleep_mutex);
	pthread_cond_broadcast(&sim->sleep_room_cond);
	pthread_mutex_unlock(&sim->sleep_mutex);
	pthread_join(sim->timer_thread, NULL);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_lock(&sim->coders[i].coder_mutex);
		pthread_cond_broadcast(&sim->coders[i].queue_cond);
		pthread_cond_broadcast(&sim->coders[i].sleep_cond);
		pthread_mutex_unlock(&sim->coders[i].coder_mutex);
		pthread_join(sim->coders[i].thread_id, NULL);
		i++;
	}
}

static int	failed_to_create_threads(t_sim *sim, int coders_created, int err)
{
	int	i;

	pull_the_fire_alarm(sim);
	pthread_mutex_lock(&sim->queue_mutex);
	sim->threads_ready = 1;
	pthread_cond_broadcast(&sim->start_cond);
	pthread_mutex_unlock(&sim->queue_mutex);
	if (err > 2)
		pthread_join(sim->monitor_thread, NULL);
	if (err > 3)
		pthread_join(sim->waiter_thread, NULL);
	i = 0;
	while (i < coders_created)
		pthread_join(sim->coders[i++].thread_id, NULL);
	return (3);
}

int	start_simulation(t_sim *sim)
{
	int	i;
	int	err;
	int	coders_created;

	coders_created = 0;
	err = create_threads(sim, &coders_created);
	if (err != 0)
		return (failed_to_create_threads(sim, coders_created, err));
	sim->start_time = get_current_time_ms();
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_lock(&sim->coders[i].coder_mutex);
		sim->coders[i].last_compile_start = sim->start_time;
		pthread_mutex_unlock(&sim->coders[i].coder_mutex);
		i++;
	}
	pthread_mutex_lock(&sim->queue_mutex);
	sim->threads_ready = 1;
	pthread_cond_broadcast(&sim->start_cond);
	pthread_mutex_unlock(&sim->queue_mutex);
	join_threads(sim);
	return (0);
}
