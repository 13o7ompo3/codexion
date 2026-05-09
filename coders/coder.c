/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:47:00 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 19:53:57 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <unistd.h>

int	is_sim_active(t_sim *sim)
{
	int	active;

	pthread_mutex_lock(&sim->state_mutex);
	active = sim->is_active;
	pthread_mutex_unlock(&sim->state_mutex);
	return (active);
}

int	all_coders_finished(t_sim *sim)
{
	int	finished;

	pthread_mutex_lock(&sim->state_mutex);
	finished = (sim->coders_remaining == 0);
	pthread_mutex_unlock(&sim->state_mutex);
	return (finished);
}

static void	coder_life_cycle(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	while (is_sim_active(sim) && !all_coders_finished(sim))
	{
		if (take_both_dongles(coder) != 0)
			break ;
		pthread_mutex_lock(&coder->coder_mutex);
		coder->compiles_done++;
		pthread_mutex_unlock(&coder->coder_mutex);
		print_compiling_sequence(coder);
		request_sleep(coder, sim->time_to_compile);
		pthread_mutex_lock(&coder->sim->state_mutex);
		if (coder->compiles_done == sim->required_compiles)
			sim->coders_remaining--;
		pthread_mutex_unlock(&coder->sim->state_mutex);
		release_both_dongles(coder);
		print_action(coder, "is debugging");
		request_sleep(coder, sim->time_to_debug);
		print_action(coder, "is refactoring");
		request_sleep(coder, sim->time_to_refactor);
	}
}

static void	wait_for_start(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	pthread_mutex_lock(&sim->queue_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->queue_mutex);
	pthread_mutex_lock(&coder->coder_mutex);
	coder->last_compile_start = sim->start_time;
	pthread_mutex_unlock(&coder->coder_mutex);
	pthread_mutex_unlock(&sim->queue_mutex);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	wait_for_start(coder);
	if (coder->sim->num_coders == 1)
	{
		print_action(coder, "has taken a dongle");
		request_sleep(coder, coder->sim->time_to_burnout + 10);
		return (NULL);
	}
	if (coder->id % 2 == 0 && coder->sim->scheduler_type == 0)
		request_sleep(coder, (coder->sim->time_to_compile
				+ coder->sim->dongle_cooldown) / 2 + 2);
	coder_life_cycle(coder);
	return (NULL);
}
