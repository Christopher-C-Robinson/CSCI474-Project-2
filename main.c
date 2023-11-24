#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_GUESTS 5
#define NUM_ROOMS 3

// Semaphores
sem_t room_sem; // Semaphore for room availability
sem_t check_in_sem; // Semaphore for check-in reservationist
sem_t check_out_sem; // Semaphore for check-out reservationist

// Guest thread function
void* guest(void* id) {
    int guest_id = *((int*)id);

    // Wait for a room to become available
    sem_wait(&room_sem);
    printf("Guest %d enters the hotel.\n", guest_id);

    // Check in
    sem_wait(&check_in_sem);
    printf("Guest %d goes to the check-in reservationist.\n", guest_id);
    // Simulate check-in process
    sleep(rand() % 3 + 1); // Random sleep to simulate processing time
    printf("Guest %d receives Room and completes check-in.\n", guest_id);
    sem_post(&check_in_sem);

    // Hotel activity
    printf("Guest %d goes to a hotel activity.\n", guest_id);
    sleep(rand() % 3 + 1); // Random sleep to simulate activity time

    // Check out
    sem_wait(&check_out_sem);
    printf("Guest %d goes to the check-out reservationist.\n", guest_id);
    // Simulate check-out process
    sleep(rand() % 3 + 1); // Random sleep to simulate processing time
    printf("Guest %d receives the receipt.\n", guest_id);
    sem_post(&check_out_sem);

    // Release the room
    sem_post(&room_sem);

    return NULL;
}

int main() {
    pthread_t guests[NUM_GUESTS];
    int guest_ids[NUM_GUESTS];
    int i;

    // Initialize semaphores
    sem_init(&room_sem, 0, NUM_ROOMS);
    sem_init(&check_in_sem, 0, 1);
    sem_init(&check_out_sem, 0, 1);

    // Create guest threads
    for(i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], NULL, guest, (void*)&guest_ids[i]);
    }

    // Wait for all guest threads to finish
    for(i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], NULL);
    }

    // Clean up
    sem_destroy(&room_sem);
    sem_destroy(&check_in_sem);
    sem_destroy(&check_out_sem);

    return 0;
}
