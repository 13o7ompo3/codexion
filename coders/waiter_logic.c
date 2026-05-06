/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   waiter_logic.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 21:20:38 by obahya            #+#    #+#             */
/*   Updated: 2026/05/01 15:37:59 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	attempt_to_reserve_dongles(t_coder *coder)
{
	if (!coder->left_dongle->reserved && !coder->right_dongle->reserved)
	{
		coder->left_dongle->reserved = coder->id;
		coder->right_dongle->reserved = coder->id;
	}
	else if (coder->left_dongle->reserved != coder->id
		|| coder->right_dongle->reserved != coder->id)
		return (-1);
	return (0);
}

static int	check_and_wake_coder(t_sim *sim, t_node *node, long long now)
{
	t_coder		*coder;
	long long	max_avail;

	coder = node->coder;
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
		pthread_mutex_lock(&coder->coder_mutex);
		coder->last_compile_start = now;
		pthread_mutex_unlock(&coder->coder_mutex);
		pthread_cond_broadcast(&coder->queue_cond);
		return (0);
	}
	max_avail = coder->left_dongle->available_at;
	if (coder->right_dongle->available_at > max_avail)
		max_avail = coder->right_dongle->available_at;
	return (max_avail - now);
}

static int	try_wake_coder(t_sim *sim, t_node *node, long long now)
{
	t_coder		*coder;

	coder = node->coder;
	if (attempt_to_reserve_dongles(coder) != 0)
		return (-1);
	return (check_and_wake_coder(sim, node, now));
}

int	process_queue_node(t_sim *sim, t_node *current, long long now,
	long long *min_wait)
{
	int	ret;

	ret = try_wake_coder(sim, current, now);
	if (ret == 0)
		return (1);
	if (ret > 0)
	{
		if (*min_wait == -1 || ret < *min_wait)
			*min_wait = ret;
	}
	return (0);
}
