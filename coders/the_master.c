/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   the_master.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:51 by obahya            #+#    #+#             */
/*   Updated: 2026/05/05 15:28:20 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <string.h>

static void	reset_reservations(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->num_coders)
	{
		sim->dongles[i].reserved = 0;
		i++;
	}
}

static int	evaluate_coders_in_queue(t_sim *sim, long long now,
	long long *min_wait)
{
	t_node	*current;
	t_node	*start_node;

	reset_reservations(sim);
	current = sim->queue;
	if (!current)
		return (0);
	*min_wait = -1;
	start_node = current;
	while (1)
	{
		if (process_queue_node(sim, current, now, min_wait))
			return (1);
		current = current->next;
		if (current == start_node)
			break ;
	}
	return (0);
}

static void	wait_for_next_event(t_sim *sim, long long min_wait)
{
	struct timespec	ts;

	if (min_wait == -1)
		pthread_cond_wait(&sim->waiter_cond, &sim->queue_mutex);
	else if (min_wait > 0)
	{
		if (min_wait < 7)
		{
			pthread_mutex_unlock(&sim->queue_mutex);
			usleep(min_wait * 1000);
			pthread_mutex_lock(&sim->queue_mutex);
			return ;
		}
		set_timespec_timeout(&ts, min_wait);
		pthread_cond_timedwait(&sim->waiter_cond, &sim->queue_mutex, &ts);
	}
}

void	*waiter_routine(void *arg)
{
	t_sim		*sim;
	long long	min_wait;

	sim = (t_sim *)arg;
	pthread_mutex_lock(&sim->queue_mutex);
	while (is_sim_active(sim))
	{
		if (!sim->queue)
		{
			pthread_cond_wait(&sim->waiter_cond, &sim->queue_mutex);
			continue ;
		}
		if (evaluate_coders_in_queue(sim, get_current_time_ms(), &min_wait))
			continue ;
		wait_for_next_event(sim, min_wait);
	}
	pthread_mutex_unlock(&sim->queue_mutex);
	return (NULL);
}
