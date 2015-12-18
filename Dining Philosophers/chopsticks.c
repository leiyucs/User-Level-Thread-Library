#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>

#include "philosopher.h"

static pthread_mutex_t chopstick_mutex[5];

/*
 * Performs necessary initialization of mutexes.
 */
void chopsticks_init(){
	int i;
	for(i = 0; i < 5; i++)
    	pthread_mutex_init(&chopstick_mutex[i], NULL); 
}

/*
 * Cleans up mutex resources.
 */
void chopsticks_destroy(){
	int i;
	for(i = 0; i < 5; i++)
    	pthread_mutex_destroy(&chopstick_mutex[i]);
}

/*
 * Uses pickup_left_chopstick and pickup_right_chopstick
 * to pick up the chopsticks
 */   
void pickup_chopsticks(int phil_id){
	//lock the chopsticks with lower index first and then the next
	int first_take, second_take;
	int rightchopstick_id = phil_id;
	int leftchopstick_id = phil_id-1 < 0 ? 4: phil_id-1;

	if (rightchopstick_id < leftchopstick_id)
	{
		first_take = rightchopstick_id;
		second_take = leftchopstick_id;
	}
	else
	{
		first_take = leftchopstick_id;
		second_take = rightchopstick_id;
	}

	pthread_mutex_lock(&chopstick_mutex[first_take]);
	if (first_take == rightchopstick_id)
	 	pickup_right_chopstick(phil_id);
	else
		pickup_left_chopstick(phil_id);

	pthread_mutex_lock(&chopstick_mutex[second_take]);
	if (second_take == rightchopstick_id)
	 	pickup_right_chopstick(phil_id);
	else
		pickup_left_chopstick(phil_id);
}

/*
 * Uses pickup_left_chopstick and pickup_right_chopstick
 * to pick up the chopsticks
 */   
void putdown_chopsticks(int phil_id){
	int rightchopstick_id = phil_id;
	int leftchopstick_id = phil_id-1 < 0 ? 4: phil_id-1;
	
	putdown_left_chopstick(phil_id);
	pthread_mutex_unlock(&chopstick_mutex[leftchopstick_id]);

	putdown_right_chopstick(phil_id);
	pthread_mutex_unlock(&chopstick_mutex[rightchopstick_id]);


}
