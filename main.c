#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_GUESTS 5
#define NUM_ROOMS 3

// Semaphores
sem_t room_sem[NUM_ROOMS];
sem_t check_in_sem;
sem_t check_out_sem;
sem_t ready_for_check_in_sem[NUM_GUESTS];
sem_t checked_in_sem[NUM_GUESTS];
sem_t ready_for_check_out_sem[NUM_GUESTS];

// Shared resources
int rooms[NUM_ROOMS] = {0}; // Room availability
int activity_counter[4] = {0}; // Counter for each activity

// Thread functions
void* guest(void* arg);
void* check_in_reservationist(void* arg);
void* check_out_reservationist(void* arg);

int main() {
    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_thread, check_out_thread;
    int i;

    // Initialize semaphores
    for (i = 0; i < NUM_ROOMS; i++) {
        sem_init(&room_sem[i], 0, 1); // Initialize each room semaphore
    }
    sem_init(&check_in_sem, 0, 1);
    sem_init(&check_out_sem, 0, 1);
    for (i = 0; i < NUM_GUESTS; i++) {
        sem_init(&ready_for_check_in_sem[i], 0, 0); // Initialize check-in semaphores for each guest
        sem_init(&checked_in_sem[i], 0, 0); // Initialize checked-in semaphores for each guest
        sem_init(&ready_for_check_out_sem[i], 0, 0); // Initialize check-out semaphores for each guest
    }

    // Create threads
    pthread_create(&check_in_thread, NULL, check_in_reservationist, NULL);
    pthread_create(&check_out_thread, NULL, check_out_reservationist, NULL);

    for (i = 0; i < NUM_GUESTS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&guests[i], NULL, guest, id);
    }

    // Join guest threads
    for (i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], NULL);
    }

    // Final accounting
    printf("\nTotal Guests: %d\n", NUM_GUESTS);
    printf("Pool: %d\n", activity_counter[0]);
    printf("Restaurant: %d\n", activity_counter[1]);
    printf("Fitness Center: %d\n", activity_counter[2]);
    printf("Business Center: %d\n", activity_counter[3]);

    // Clean up
    for (i = 0; i < NUM_ROOMS; i++) {
        sem_destroy(&room_sem[i]);
    }
    sem_destroy(&check_in_sem);
    sem_destroy(&check_out_sem);
    for (i = 0; i < NUM_GUESTS; i++) {
        sem_destroy(&ready_for_check_in_sem[i]);
        sem_destroy(&checked_in_sem[i]);
        sem_destroy(&ready_for_check_out_sem[i]);
    }

    return 0;
}

void* guest(void* arg) {
    int id = *(int*)arg;
    char* activities[] = {"swimming pool", "restaurant", "fitness center", "business center"};
    int activity_index;

    printf("Guest %d enters the hotel\n", id); // Guest enters the hotel
    // Indicate readiness for check-in
    printf("Guest %d goes to the check-in reservationist\n", id); // Guest goes to the check-in reservationist
    sem_post(&ready_for_check_in_sem[id]);

    // Wait for room assignment
    sem_wait(&checked_in_sem[id]);

    // Hotel activity
    activity_index = rand() % 4;
    printf("Guest %d goes to the %s\n", id, activities[activity_index]);
    activity_counter[activity_index]++;
    sleep(rand() % 3 + 1);

    // Indicate readiness for check-out
    printf("Guest %d goes to the check-out reservationist\n", id); // Guest goes to the check-out reservationist
    sem_post(&ready_for_check_out_sem[id]);

    // Wait for check-out completion
    sem_wait(&checked_in_sem[id]); // Reuse this semaphore for check-out completion

    free(arg);
    return NULL;
}

void* check_in_reservationist(void* arg) {
    int currentRoom = 0;
    while (1) {
        for (int i = 0; i < NUM_GUESTS; i++) {
            sem_wait(&ready_for_check_in_sem[i]); // Wait for a guest to be ready
            sem_wait(&room_sem[currentRoom]); // Acquire a room
            sem_wait(&check_in_sem);
            printf("The check-in reservationist greets Guest %d and assigns Room %d\n", i, currentRoom);
            rooms[currentRoom] = 1; // Mark room as occupied
            sem_post(&checked_in_sem[i]); // Indicate that the guest is checked in
            sem_post(&check_in_sem);
            currentRoom = (currentRoom + 1) % NUM_ROOMS;
        }
    }
    return NULL;
}

void* check_out_reservationist(void* arg) {
    while (1) {
        for (int i = 0; i < NUM_GUESTS; i++) {
            sem_wait(&ready_for_check_out_sem[i]); // Wait for a guest to be ready for check-out
            sem_wait(&check_out_sem);
            printf("The check-out reservationist greets Guest %d and receives the key\n", i);
            printf("The receipt was printed.\n"); // Print the receipt
            for (int j = 0; j < NUM_ROOMS; j++) {
                if (rooms[j] == 1) { // Find the occupied room
                    rooms[j] = 0; // Mark room as available
                    sem_post(&room_sem[j]); // Release room semaphore
                    break;
                }
            }
            sem_post(&checked_in_sem[i]); // Use this semaphore to indicate check-out completion
            sem_post(&check_out_sem);
        }
    }
    return NULL;
}