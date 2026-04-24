/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   the_master.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:51 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 18:43:52 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <string.h>

static int	try_wake_coder(t_sim *sim, t_node *node, long long now)
{
	t_coder	*coder = node->coder;
	long long	max_avail;

	if (!coder->left_dongle->reserved && !coder->right_dongle->reserved)
	{
		coder->left_dongle->reserved = coder->id;
		coder->right_dongle->reserved = coder->id;
	}
	else if (coder->left_dongle->reserved != coder->id
		|| coder->right_dongle->reserved != coder->id)
		return (-1);
	if (coder->left_dongle->in_use || coder->right_dongle->in_use)
		return (-1);

	if (coder->left_dongle->available_at <= now
		&& coder->right_dongle->available_at <= now)
	{
		remove_node(&sim->queue, node);
		coder->owns_hardware = 1;
		coder->left_dongle->in_use = 1;
		coder->right_dongle->in_use = 1;
		coder->left_dongle->reserved = 0;
		coder->right_dongle->reserved = 0;
		pthread_cond_signal(&coder->queue_cond);
		return (0);
	}
	max_avail = coder->left_dongle->available_at;
	if (coder->right_dongle->available_at > max_avail)
		max_avail = coder->right_dongle->available_at;
	return (max_avail - now);
}

void	*waiter_routine(void *arg)
{
	t_sim		*sim = (t_sim *)arg;
	t_node		*current;
	int			ret;
	long long	frozen_now;
	long long	min_wait;
	int			somebody_woke;
	int			i;

	pthread_mutex_lock(&sim->queue_mutex);
	while (sim->is_active)
	{
		if (!sim->queue)
		{
			pthread_cond_wait(&sim->waiter_cond, &sim->queue_mutex);
			continue;
		}
		
		current = sim->queue;
		frozen_now = get_current_time_ms();
		min_wait = -1;
		somebody_woke = 0;

		i = 0;
		while (i < sim->num_coders)
		{
			sim->dongles[i].reserved = 0;
			i++;
		}

		do
		{
			ret = try_wake_coder(sim, current, frozen_now);
			if (ret == 0)
			{
				somebody_woke = 1;
				// break;
				current = sim->queue;
				continue;
			}
			else if (ret > 0)
			{
				if (min_wait == -1 || ret < min_wait)
					min_wait = ret;
			}
			current = current->next;
		} while (current && current != sim->queue);

		if (somebody_woke)
			continue;

		if (min_wait == -1)
			pthread_cond_wait(&sim->waiter_cond, &sim->queue_mutex);
		else
		{
			struct timespec ts;
			set_timespec_timeout(&ts, min_wait);
			pthread_cond_timedwait(&sim->waiter_cond, &sim->queue_mutex, &ts);
		}
	}
	pthread_mutex_unlock(&sim->queue_mutex);
	return (NULL);
}