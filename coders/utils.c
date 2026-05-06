/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:44:02 by obahya            #+#    #+#             */
/*   Updated: 2026/05/05 15:28:56 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdio.h>
#include <sys/time.h>
#include <stddef.h>
#include <string.h>

long long	get_current_time_ms(void)
{
	struct timeval	tv;
	long long		time_in_ms;

	if (gettimeofday(&tv, NULL) == -1)
		return (-1);
	time_in_ms = (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
	return (time_in_ms);
}

void	print_action(t_coder *coder, char *action)
{
	long long	current_time;

	pthread_mutex_lock(&coder->sim->write_mutex);
	if (is_sim_active(coder->sim) || strcmp(action, "burned out") == 0)
	{
		current_time = get_current_time_ms() - coder->sim->start_time;
		printf("%lld %d %s\n", current_time, coder->id, action);
	}
	pthread_mutex_unlock(&coder->sim->write_mutex);
}

void	print_compiling_sequence(t_coder *coder)
{
	long long	current_time;

	pthread_mutex_lock(&coder->sim->write_mutex);
	if (is_sim_active(coder->sim))
	{
		current_time = get_current_time_ms() - coder->sim->start_time;
		printf("%lld %d has taken a dongle\n", current_time, coder->id);
		printf("%lld %d has taken a dongle\n", current_time, coder->id);
		printf("%lld %d is compiling\n", current_time, coder->id);
	}
	pthread_mutex_unlock(&coder->sim->write_mutex);
}

void	wake_up_coders(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->num_coders)
	{
		pthread_cond_broadcast(&sim->coders[i].queue_cond);
		pthread_cond_broadcast(&sim->coders[i].sleep_cond);
		i++;
	}
}
