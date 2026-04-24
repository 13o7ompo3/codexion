/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 02:36:45 by obahya            #+#    #+#             */
/*   Updated: 2026/04/24 17:08:37 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void	pull_the_fire_alarm(t_sim *sim)
{
	int	i;

	pthread_mutex_lock(&sim->queue_mutex);
	pthread_mutex_lock(&sim->sleep_mutex);
	pthread_mutex_lock(&sim->write_mutex);
	sim->is_active = 0;
	pthread_cond_broadcast(&sim->waiter_cond);
	pthread_cond_broadcast(&sim->sleep_room_cond);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_cond_broadcast(&sim->coders[i].queue_cond);
		pthread_cond_broadcast(&sim->coders[i].sleep_cond);
		i++;
	}
	pthread_mutex_unlock(&sim->write_mutex);
	pthread_mutex_unlock(&sim->sleep_mutex);
	pthread_mutex_unlock(&sim->queue_mutex);
}

void *monitor_routine(void *arg)
{
	t_sim *sim = (t_sim *)arg;
	int i;
	int all_compiled;

	pthread_mutex_lock(&sim->queue_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->queue_mutex);
	pthread_mutex_unlock(&sim->queue_mutex);
	while (1)
	{
		i = 0;
		all_compiled = 1;
		while (i < sim->num_coders)
		{
			pthread_mutex_lock(&sim->coders[i].coder_mutex);
			if ((get_current_time_ms() - sim->coders[i].last_compile_start) > sim->time_to_burnout)
			{
				pthread_mutex_unlock(&sim->coders[i].coder_mutex);
				pull_the_fire_alarm(sim);
				print_action(&sim->coders[i], "burned out");
				return (NULL);
			}
			if (sim->required_compiles == -1 || sim->coders[i].compiles_done < sim->required_compiles)
				all_compiled = 0;
			pthread_mutex_unlock(&sim->coders[i].coder_mutex);
			i++;
		}
		if (sim->required_compiles != -1 && all_compiled == 1)
		{
			pull_the_fire_alarm(sim);
			return (NULL);
		}
		usleep(1000); 
	}
	return (NULL);
}

int start_simulation(t_sim *sim)
{
	int         i;

	i = 0;
	while (i < sim->num_coders)
	{
		if (pthread_create(&sim->coders[i].thread_id, NULL, &coder_routine, &sim->coders[i]) != 0)
			return (1);
		i++;
	}
	if (pthread_create(&sim->monitor_thread, NULL, &monitor_routine, sim) != 0)
		return (1);
	if (pthread_create(&sim->waiter_thread, NULL, &waiter_routine, sim) != 0)
		return (1);
	if (pthread_create(&sim->timer_thread, NULL, &sleep_room_routine, sim) != 0)
		return (1);
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
	pthread_join(sim->monitor_thread, NULL);
	pthread_mutex_lock(&sim->queue_mutex);
	pthread_cond_broadcast(&sim->waiter_cond);
	pthread_mutex_unlock(&sim->queue_mutex);
	pthread_join(sim->waiter_thread, NULL);
	// Fix: Use the correct mutex for the condition variable.
	pthread_mutex_lock(&sim->sleep_mutex);
	pthread_cond_broadcast(&sim->sleep_room_cond);
	pthread_mutex_unlock(&sim->sleep_mutex);
	pthread_join(sim->timer_thread, NULL);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_join(sim->coders[i].thread_id, NULL);
		i++;
	}
	return (0);
}

void cleanup_simulation(t_sim *sim)
{
	int i;
	
	if (sim->coders)
	{
		i = 0;
		while (i < sim->num_coders)
		{
			pthread_cond_destroy(&sim->coders[i].queue_cond);
			pthread_cond_destroy(&sim->coders[i].sleep_cond);
			pthread_mutex_destroy(&sim->coders[i].coder_mutex);
			i++;
		}
		free(sim->coders);
	}
	pthread_cond_destroy(&sim->start_cond);
	pthread_cond_destroy(&sim->waiter_cond);
	pthread_mutex_destroy(&sim->write_mutex);
	pthread_mutex_destroy(&sim->queue_mutex);
	pthread_mutex_destroy(&sim->sleep_mutex);
	pthread_cond_destroy(&sim->sleep_room_cond);
	if (sim->dongles)
		free(sim->dongles);
	if (sim->sleep_heap)
		free_heap(sim->sleep_heap);
}

int	main(int argc, char **argv)
{
	t_sim	*sim;
	int		error;

	sim = malloc(sizeof(t_sim));
	if (!sim)
		return (2);
	error = parse_args(sim, argc, argv);
	if (error == 1)
		printf("Invalid arguments, usage: ./codexion <num_coders> "
			"<time_to_burnout> <time_to_compile> <time_to_debug> "
			"<time_to_refactor> <required_compiles> <dongle_cooldown> "
			"<scheduler_type> %d\n", argc);
	else if (error == 2)
		printf("Failed to initialize simulation\n");
	else
	{
		error = start_simulation(sim);
		if (error == 1)
			printf("Error: Failed to start simulation\n");
		printf("Simulation ended\n");
	}
	cleanup_simulation(sim);
	free(sim);
	return (error);
}
