#include "codexion.h"
#include <unistd.h>

void	precise_sleep(long long time_in_ms, t_sim *sim)
{
	long long start_time;
	long long current_time;

	start_time = get_current_time_ms();
	while (1)
	{
		// 1. Check if the simulation ended (e.g., someone else burned out)
		pthread_mutex_lock(&sim->state_mutex);
		if (!sim->is_active)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			break ;
		}
		pthread_mutex_unlock(&sim->state_mutex);
		// 2. Check if we have slept long enough
		current_time = get_current_time_ms();
		if ((current_time - start_time) >= time_in_ms)
			break ;
		// 3. Sleep for a tiny fraction of a millisecond to save CPU
		// usleep(500); 
	}
}

int take_dongle(t_coder *coder, t_dongle *dongle)
{
    t_node *my_node;
    
    my_node = malloc(sizeof(t_node));
    if (!my_node) return (1);
    my_node->coder = coder;
    my_node->next = NULL;
    my_node->prev = NULL;

    pthread_mutex_lock(&dongle->mutex);
    enqueue(dongle, my_node, coder->sim->scheduler_type);

    while (1)
    {
        // 1. Check if the monitor stopped the simulation
        pthread_mutex_lock(&coder->sim->state_mutex);
        if (!coder->sim->is_active)
        {
            pthread_mutex_unlock(&coder->sim->state_mutex);
            pthread_mutex_unlock(&dongle->mutex);
            // We leave my_node in the queue; cleanup_simulation will free() it safely.
            return (1); 
        }
        pthread_mutex_unlock(&coder->sim->state_mutex);

        // 2. Check if it's our turn
        if (dongle->queue == my_node && !dongle->is_held)
        {
            if (get_current_time_ms() >= dongle->available_at)
                break ; // It's our turn and the cooldown is over!
            
            // It's our turn, but the dongle is on cooldown.
            // Unlock, sleep for half a millisecond, lock, and check the time again.
            pthread_mutex_unlock(&dongle->mutex);
            usleep(500); 
            pthread_mutex_lock(&dongle->mutex);
            continue ;
        }
        
        // 3. Someone else has it, or we aren't next in line. Wait for a broadcast.
        pthread_cond_wait(&dongle->cond, &dongle->mutex);
    }

    dequeue(dongle);
    free(my_node); // Successfully removed, free the memory
    
    dongle->is_held = 1;
    print_action(coder, "has taken a dongle");
    pthread_mutex_unlock(&dongle->mutex);
    return (0);
}

void	release_dongle(t_coder *coder, t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->is_held = 0;
	dongle->available_at = get_current_time_ms() + coder->sim->dongle_cooldown;
	// 3. Wake up everyone waiting in the queue so they can check if it's their turn
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->mutex);
}


void	*coder_routine(void *arg)
{
	t_coder		*coder = (t_coder *)arg;
	t_sim		*sim = coder->sim;
	t_dongle	*first;
	t_dongle	*second;

	pthread_mutex_lock(&sim->state_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->state_mutex);
	pthread_mutex_unlock(&sim->state_mutex);

	if (sim->num_coders == 1)
	{
		take_dongle(coder, coder->left_dongle);
		precise_sleep(sim->time_to_burnout + 10, sim); 
		return (NULL);
	}

	if (coder->left_dongle < coder->right_dongle)
	{
		first = coder->left_dongle;
		second = coder->right_dongle;
	}
	else
	{
		first = coder->right_dongle;
		second = coder->left_dongle;
	}

	while (1)
	{
		pthread_mutex_lock(&sim->state_mutex);
		if (!sim->is_active)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			break ;
		}
		pthread_mutex_unlock(&sim->state_mutex);

		if (take_dongle(coder, first) != 0) break;
		if (take_dongle(coder, second) != 0)
		{
			release_dongle(coder, first);
			break;
		}
		// if (coder->id % 2 == 0)
		// {
		// 	if (take_dongle(coder, coder->right_dongle) != 0) break;
		// 	if (take_dongle(coder, coder->left_dongle) != 0) 
		// 	{ 
		// 		release_dongle(coder, coder->right_dongle); 
		// 		break; 
		// 	}
		// }
		// else
		// {
		// 	if (take_dongle(coder, coder->left_dongle) != 0) break;
		// 	if (take_dongle(coder, coder->right_dongle) != 0) 
		// 	{ 
		// 		release_dongle(coder, coder->left_dongle); 
		// 		break; 
		// 	}
		// }

		pthread_mutex_lock(&sim->state_mutex);
		coder->last_compile_start = get_current_time_ms();
		coder->compiles_done++;
		pthread_mutex_unlock(&sim->state_mutex);

		print_action(coder, "is compiling");
		precise_sleep(sim->time_to_compile, sim);

		release_dongle(coder, coder->left_dongle);
		release_dongle(coder, coder->right_dongle);

		print_action(coder, "is debugging");
		precise_sleep(sim->time_to_debug, sim);

		print_action(coder, "is refactoring");
		precise_sleep(sim->time_to_refactor, sim);
	}
	return (NULL);
}

