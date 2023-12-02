#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_GUESTS 5
#define NUM_ROOMS 3

// Semaphores
sem_t room_sem;
sem_t check_in_sem;
sem_t check_out_sem;

// Shared data
int available_rooms[NUM_ROOMS] = {0, 1, 2}; // Initialize all rooms as available
int activity_count[4] = {0}; // To count activities: pool, restaurant, fitness center, business center

// Randomly choose an activity for guests
const char *activities[] = {"swimming pool", "restaurant", "fitness center", "business center"};

// Guest thread routine
void* guest_routine(void* arg) {
    int guest_id = *(int*)arg;
    int room_number = -1;

    // Try to enter the hotel and check in
    sem_wait(&room_sem);
    printf("Guest %d enters the hotel.\n", guest_id);

    // Check-in process
    sem_wait(&check_in_sem);
    printf("Guest %d goes to the check-in reservationist.\n", guest_id);

    // Assign a room to the guest
    for (int i = 0; i < NUM_ROOMS; i++) {
        if (available_rooms[i] != -1) {
            room_number = available_rooms[i];
            available_rooms[i] = -1; // Mark room as occupied
            printf("Guest %d receives Room %d and completes check-in.\n", guest_id, room_number);
            break;
        }
    }

    // If no room was available, wait
    while (room_number == -1) {
        for (int i = 0; i < NUM_ROOMS; i++) {
            if (available_rooms[i] != -1) {
                room_number = available_rooms[i];
                available_rooms[i] = -1; // Mark room as occupied
                printf("Guest %d receives Room %d and completes check-in.\n", guest_id, room_number);
                break;
            }
        }
        if (room_number == -1) {
            sem_post(&check_in_sem); // Allow other guests to check in
            sem_wait(&room_sem); // Wait for a room to become available
            sem_wait(&check_in_sem); // Try to get a room again
        }
    }

    sem_post(&check_in_sem);

    // Guest activity
    int activity_index = rand() % 4;
    printf("Guest %d goes to the %s.\n", guest_id, activities[activity_index]);
    activity_count[activity_index]++;
    sleep(rand() % 3 + 1);

    // Check-out process
    sem_wait(&check_out_sem);
    printf("Guest %d goes to the check-out reservationist and returns room %d.\n", guest_id, room_number);
    available_rooms[room_number] = room_number; // Mark room as available
    printf("Guest %d receives the receipt.\n", guest_id);
    sem_post(&check_out_sem);
    sem_post(&room_sem); // Room is now available

    return NULL;
}

// Check-in reservationist routine
void* check_in_routine(void* arg) {
    // The check-in reservationist should be idle (blocked) if no guests are waiting
    while (1) {
        sem_wait(&check_in_sem); // Will be released by guest or at the end of simulation
        printf("The check-in reservationist is ready to greet a guest.\n");
        sem_post(&check_in_sem);
        sleep(1); // Wait a moment to simulate the processing time
    }
    return NULL;
}

// Check-out reservationist routine
void* check_out_routine(void* arg) {
    // The check-out reservationist should be idle (blocked) if no guests are waiting
    while (1) {
        sem_wait(&check_out_sem); // Will be released by guest or at the end of simulation
        printf("The check-out reservationist is ready to process a guest.\n");
        sem_post(&check_out_sem);
        sleep(1); // Wait a moment to simulate the processing time
    }
    return NULL;
}

int main() {
    srand(time(NULL)); // Seed the random number generator

    // Initialize semaphores
    sem_init(&room_sem, 0, NUM_ROOMS);
    sem_init(&check_in_sem, 0, 1);
    sem_init(&check_out_sem, 0, 1);

    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_res;
    pthread_t check_out_res;
    int guest_ids[NUM_GUESTS];

    // Create check-in and check-out reservationist threads
    pthread_create(&check_in_res, NULL, check_in_routine, NULL);
    pthread_create(&check_out_res, NULL, check_out_routine, NULL);

    // Create guest threads
    for (int i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], NULL, guest_routine, &guest_ids[i]);
    }

    // Join guest threads
    for (int i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], NULL);
    }

    // Terminate reservationist threads
    pthread_cancel(check_in_res);
    pthread_cancel(check_out_res);

    // Destroy semaphores
    sem_destroy(&room_sem);
    sem_destroy(&check_in_sem);
    sem_destroy(&check_out_sem);

    // Accounting
    printf("\nTotal Guests: %d\n", NUM_GUESTS);
    printf("Pool: %d\n", activity_count[0]);
    printf("Restaurant: %d\n", activity_count[1]);
    printf("Fitness Center: %d\n", activity_count[2]);
    printf("Business Center: %d\n", activity_count[3]);

    return 0;
}
