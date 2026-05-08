/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/08 17:32:34 by obahya            #+#    #+#             */
/*   Updated: 2026/05/08 17:33:10 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	destroy_sim_mutex_and_cond(t_sim *sim)
{
	if (sim->main_init_step > 0)
		pthread_cond_destroy(&sim->start_cond);
	if (sim->main_init_step > 1)
		pthread_mutex_destroy(&sim->write_mutex);
	if (sim->main_init_step > 2)
		pthread_mutex_destroy(&sim->queue_mutex);
	if (sim->main_init_step > 3)
		pthread_cond_destroy(&sim->waiter_cond);
	if (sim->main_init_step > 4)
		pthread_mutex_destroy(&sim->sleep_mutex);
	if (sim->main_init_step > 5)
		pthread_cond_destroy(&sim->sleep_room_cond);
	if (sim->main_init_step > 6)
		pthread_mutex_destroy(&sim->state_mutex);
}

void	cleanup_simulation(t_sim *sim)
{
	int	i;

	destroy_sim_mutex_and_cond(sim);
	if (sim->coders)
	{
		i = 0;
		while (sim->coders_init_num > 0)
		{
			if (sim->coders_init_num > 0)
				pthread_mutex_destroy(&sim->coders[i].coder_mutex);
			if (sim->coders_init_num > 1)
				pthread_cond_destroy(&sim->coders[i].queue_cond);
			if (sim->coders_init_num > 2)
				pthread_cond_destroy(&sim->coders[i].sleep_cond);
			sim->coders_init_num -= 3;
			i++;
		}
		free(sim->coders);
	}
	if (sim->dongles)
		free(sim->dongles);
	if (sim->sleep_heap)
		free_heap(sim->sleep_heap);
}
