/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_actions.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 21:37:09 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 17:48:34 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	set_deadline(t_coder *coder)
{
	pthread_mutex_lock(&coder->coder_mutex);
	if (coder->sim->scheduler_type == 0)
		coder->deadline = coder->last_compile_start
			+ coder->sim->time_to_burnout;
	else
		coder->deadline = get_current_time_ms();
	pthread_mutex_unlock(&coder->coder_mutex);
}

int	take_both_dongles(t_coder *coder)
{
	set_deadline(coder);
	pthread_mutex_lock(&coder->sim->queue_mutex);
	coder->sim->queue = append_node(coder->sim->queue, &coder->node,
			compare_deadline);
	pthread_cond_broadcast(&coder->sim->waiter_cond);
	while (is_sim_active(coder->sim) && !coder->owns_hardware)
		pthread_cond_wait(&coder->queue_cond, &coder->sim->queue_mutex);
	if (!is_sim_active(coder->sim))
	{
		if (!coder->owns_hardware)
			remove_node(&coder->sim->queue, &coder->node);
		pthread_mutex_unlock(&coder->sim->queue_mutex);
		if (coder->owns_hardware)
			release_both_dongles(coder);
		return (1);
	}
	pthread_mutex_unlock(&coder->sim->queue_mutex);
	return (0);
}

void	release_both_dongles(t_coder *coder)
{
	long long	now;

	now = get_current_time_ms();
	pthread_mutex_lock(&coder->sim->queue_mutex);
	coder->owns_hardware = 0;
	coder->left_dongle->available_at = now + coder->sim->dongle_cooldown;
	coder->right_dongle->available_at = coder->left_dongle->available_at;
	coder->left_dongle->in_use = 0;
	coder->right_dongle->in_use = 0;
	pthread_mutex_unlock(&coder->sim->queue_mutex);
	pthread_cond_broadcast(&coder->sim->waiter_cond);
}
