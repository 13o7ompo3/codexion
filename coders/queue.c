#include "codexion.h"

// t_node *dequeue(t_dongle *dongle)
// {
// 	t_node *head;
// 	t_node *tail;

// 	if (!dongle->queue)
// 		return (NULL);
// 	head = (t_node *)dongle->queue;
// 	if (head->next == head)
// 		dongle->queue = NULL;
// 	else
// 	{
// 		tail = head->prev;
// 		dongle->queue = head->next;
// 		tail->next = dongle->queue;
// 		((t_node *)dongle->queue)->prev = tail;
// 	}
// 	head->next = NULL;
// 	head->prev = NULL;
// 	return (head);
// }

long long get_deadline(t_coder *coder)
{
	return (coder->last_compile_start + coder->sim->time_to_burnout);
}

void enqueue(t_sim *dongle, t_node *new_node, int scheduler_type)
{
	t_node *curr;
	long long	d_new;
	long long	d_curr;

	if (!dongle->queue)
	{
		dongle->queue = new_node;
		new_node->next = new_node;
		new_node->prev = new_node;
		return ;
	}
	curr = (t_node *)dongle->queue;
	if (scheduler_type == 1)
	{
        d_new = get_deadline(new_node->coder);
        do {
            d_curr = get_deadline(curr->coder);

            // Standard EDF condition
            if (d_new < d_curr)
                break ;
            
            // THE TIE-BREAKER: If deadlines are exactly equal, prioritize the coder with fewer compiles
            if (d_new == d_curr && new_node->coder->compiles_done < curr->coder->compiles_done)
                break ;
                
            curr = curr->next;
        } while (curr != (t_node *)dongle->queue);
	}
	new_node->next = curr;
	new_node->prev = curr->prev;
	curr->prev->next = new_node;
	curr->prev = new_node;
    if (scheduler_type == 1 && curr == (t_node *)dongle->queue)
    {
        d_curr = get_deadline(curr->coder);
        if (d_new < d_curr || (d_new == d_curr && new_node->coder->compiles_done < curr->coder->compiles_done))
        {
            dongle->queue = new_node; // We are the new head!
        }
    }
}

void remove_node(t_sim *sim, t_node *node)
{
	if (node->next == node)
	{
		sim->queue = NULL;
	}
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
		if (sim->queue == node)
			sim->queue = node->next;
	}
	node->next = NULL;
	node->prev = NULL;
}

int take_both_dongles(t_coder *coder)
{
    t_node *my_node = malloc(sizeof(t_node));
    if (!my_node) return (1);
    my_node->coder = coder;

    pthread_mutex_lock(&coder->sim->state_mutex);
    enqueue(coder->sim, my_node, coder->sim->scheduler_type);

    while (1)
    {
        if (!coder->sim->is_active)
        {
            remove_node(coder->sim, my_node);
            free(my_node);
            pthread_mutex_unlock(&coder->sim->state_mutex);
            return (1);
        }

        // 1. Check if anyone with HIGHER PRIORITY needs my dongles
        int conflict = 0;
        t_node *curr = (t_node *)coder->sim->queue;
        if (curr)
        {
            while (curr != my_node)
            {
                if (curr->coder->left_dongle == coder->left_dongle || 
                    curr->coder->left_dongle == coder->right_dongle ||
                    curr->coder->right_dongle == coder->left_dongle ||
                    curr->coder->right_dongle == coder->right_dongle)
                {
                    conflict = 1;
                    break;
                }
                curr = curr->next;
            }
        }

        int on_cooldown = 0; // ADDED FLAG

        // 2. If no higher-priority coder needs them, check if they are free
        if (!conflict)
        {
            t_dongle *first = coder->left_dongle < coder->right_dongle ? coder->left_dongle : coder->right_dongle;
            t_dongle *second = coder->left_dongle < coder->right_dongle ? coder->right_dongle : coder->left_dongle;
            
            pthread_mutex_lock(&first->mutex);
            pthread_mutex_lock(&second->mutex);
            
            // They are not being actively held
            if (!first->is_held && !second->is_held)
            {
                // The cooldown has passed!
                if (get_current_time_ms() >= first->available_at &&
                    get_current_time_ms() >= second->available_at)
                {
                    first->is_held = 1;
                    second->is_held = 1;
                    pthread_mutex_unlock(&second->mutex);
                    pthread_mutex_unlock(&first->mutex);
                    break; 
                }
                else
                {
                    // It's our turn, but we are waiting on the clock
                    on_cooldown = 1; 
                }
            }
            pthread_mutex_unlock(&second->mutex);
            pthread_mutex_unlock(&first->mutex);
        }
        
        // 3. Sleep Logic
        if (on_cooldown)
        {
            // Unlock the state, sleep for half a millisecond, and loop to check the clock again
            pthread_mutex_unlock(&coder->sim->state_mutex);
            usleep(500);
            pthread_mutex_lock(&coder->sim->state_mutex);
        }
        else
        {
            // We have a conflict (someone else's turn or held). 
            // Deep sleep until someone actively releases dongles.
            pthread_cond_wait(&coder->sim->arbiter_cond, &coder->sim->state_mutex);
        }
    }

    remove_node(coder->sim, my_node);
    free(my_node);
    
    long long current_time = get_current_time_ms() - coder->sim->start_time;
    printf("%lld %d has taken a dongle\n", current_time, coder->id);
    printf("%lld %d has taken a dongle\n", current_time, coder->id);
    
    pthread_mutex_unlock(&coder->sim->state_mutex);
    return (0);
}

void release_both_dongles(t_coder *coder)
{
    long long current = get_current_time_ms();
    long long next_avail = current + coder->sim->dongle_cooldown;
    
    t_dongle *first = coder->left_dongle < coder->right_dongle ? coder->left_dongle : coder->right_dongle;
    t_dongle *second = coder->left_dongle < coder->right_dongle ? coder->right_dongle : coder->left_dongle;
    
    pthread_mutex_lock(&first->mutex);
    pthread_mutex_lock(&second->mutex);
    
    first->is_held = 0;
    first->available_at = next_avail;
    second->is_held = 0;
    second->available_at = next_avail;
    
    pthread_mutex_unlock(&second->mutex);
    pthread_mutex_unlock(&first->mutex);
    
    // Wake up everyone in the global queue so they can check priorities again
    pthread_mutex_lock(&coder->sim->state_mutex);
    pthread_cond_broadcast(&coder->sim->arbiter_cond);
    pthread_mutex_unlock(&coder->sim->state_mutex);
}
