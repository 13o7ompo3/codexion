/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sleep_room.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/24 18:43:44 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 16:33:11 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <sys/time.h>

void	request_sleep(t_coder *coder, long long duration_ms)
{
	if (duration_ms == 0)
		return ;
	pthread_mutex_lock(&coder->sim->queue_mutex);
	if (!is_sim_active(coder->sim))
	{
		pthread_mutex_unlock(&coder->sim->queue_mutex);
		return ;
	}
	pthread_mutex_lock(&coder->coder_mutex);
	coder->is_sleeping = 1;
	pthread_mutex_unlock(&coder->coder_mutex);
	pthread_mutex_unlock(&coder->sim->queue_mutex);
	pthread_mutex_lock(&coder->sim->sleep_mutex);
	coder->wake_up_time = get_current_time_ms() + duration_ms;
	heap_insert(coder->sim->sleep_heap, coder);
	pthread_cond_broadcast(&coder->sim->sleep_room_cond);
	pthread_mutex_unlock(&coder->sim->sleep_mutex);
	pthread_mutex_lock(&coder->coder_mutex);
	while (coder->is_sleeping)
		pthread_cond_wait(&coder->sleep_cond, &coder->coder_mutex);
	pthread_mutex_unlock(&coder->coder_mutex);
}

static int	process_awakenings(t_sim *sim, long long now)
{
	t_coder	*top;
	int		woke_someone;

	woke_someone = 0;
	while (sim->sleep_heap->size > 0)
	{
		top = sim->sleep_heap->array[0];
		if (top->wake_up_time > now)
			break ;
		heap_remove_at(sim->sleep_heap, 0);
		pthread_mutex_lock(&top->coder_mutex);
		top->is_sleeping = 0;
		pthread_mutex_unlock(&top->coder_mutex);
		pthread_cond_broadcast(&top->sleep_cond);
		woke_someone = 1;
	}
	return (woke_someone);
}

void	set_timespec_timeout(struct timespec *ts, long long delay_in_ms)
{
	struct timeval	tv;
	long long		absolute_usec;

	if (delay_in_ms < 0)
		delay_in_ms = 0;
	gettimeofday(&tv, NULL);
	absolute_usec = (tv.tv_sec * 1000000LL) + tv.tv_usec
		+ (delay_in_ms * 1000LL);
	ts->tv_sec = absolute_usec / 1000000LL;
	ts->tv_nsec = (absolute_usec % 1000000LL) * 1000LL;
}

static void	sleep_room_wait(t_sim *sim, long long now)
{
	struct timespec	ts;
	long long		time_until_next;

	time_until_next = sim->sleep_heap->array[0]->wake_up_time - now;
	if (time_until_next <= 0)
		return ;
	if (time_until_next < 10)
	{
		pthread_mutex_unlock(&sim->sleep_mutex);
		usleep(time_until_next << 9);
		pthread_mutex_lock(&sim->sleep_mutex);
	}
	else
	{
		set_timespec_timeout(&ts, time_until_next - 2);
		pthread_cond_timedwait(&sim->sleep_room_cond, &sim->sleep_mutex, &ts);
	}
}

void	*sleep_room_routine(void *arg)
{
	t_sim			*sim;
	long long		now;

	sim = (t_sim *)arg;
	pthread_mutex_lock(&sim->sleep_mutex);
	while (is_sim_active(sim))
	{
		if (sim->sleep_heap->size == 0)
		{
			pthread_cond_wait(&sim->sleep_room_cond, &sim->sleep_mutex);
			continue ;
		}
		now = get_current_time_ms();
		if (!process_awakenings(sim, now))
			sleep_room_wait(sim, now);
	}
	pthread_mutex_unlock(&sim->sleep_mutex);
	return (NULL);
}
