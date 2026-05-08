/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 21:08:20 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 11:16:14 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	is_burnout(t_coder *coder, long long now)
{
	pthread_mutex_lock(&coder->coder_mutex);
	if ((now - coder->last_compile_start)
		>= coder->sim->time_to_burnout)
	{
		pthread_mutex_unlock(&coder->coder_mutex);
		return (1);
	}
	pthread_mutex_unlock(&coder->coder_mutex);
	return (0);
}

static int	check_simulation_state(t_sim *sim, long long now)
{
	int	i;
	int	all_compiled;

	i = 0;
	all_compiled = 1;
	while (i < sim->num_coders)
	{
		if (is_burnout(&sim->coders[i], now) && !all_coders_finished(sim))
		{
			pull_the_fire_alarm(sim);
			print_action(&sim->coders[i], "burned out");
			return (1);
		}
		i++;
	}
	if (all_coders_finished(sim))
	{
		pull_the_fire_alarm(sim);
		return (1);
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_sim		*sim;
	long long	now;

	sim = (t_sim *)arg;
	pthread_mutex_lock(&sim->queue_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->queue_mutex);
	pthread_mutex_unlock(&sim->queue_mutex);
	while (is_sim_active(sim))
	{
		now = get_current_time_ms();
		if (check_simulation_state(sim, now))
			return (NULL);
		usleep(900);
	}
	return (NULL);
}
