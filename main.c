#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_GUESTS 5
#define NUM_ROOMS 3
#define NUM_ACTIVITIES 4

// Semaphores
sem_t room_sem, check_in_sem, check_out_sem, activity_sem;

// Shared resources and counters
int rooms[NUM_ROOMS] = {0};
int activity_counters[NUM_ACTIVITIES] = {0};
const char* activity_names[] = {"Swimming Pool", "Restaurant", "Fitness Center", "Business Center"};

int guests_checked_out = 0;

// Function to choose a random activity for a guest
int choose_activity() {
    return rand() % NUM_ACTIVITIES;
}

// Guest thread function
void* guest(void* id) {
    int guest_id = *((int*)id);
    int room_assigned = -1;

    // Try to get a room
    while (room_assigned == -1) {
        sem_wait(&room_sem);
        for (int i = 0; i < NUM_ROOMS; i++) {
            if (rooms[i] == 0) {
                rooms[i] = 1;
                room_assigned = i;
                printf("Guest %d enters the hotel and is assigned room %d.\n", guest_id, room_assigned);
                break;
            }
        }
        if (room_assigned == -1) {
            // If no room is available, release semaphore and try again
            sem_post(&room_sem);
        }
    }

    // Check-in process
    sem_wait(&check_in_sem);
    printf("Guest %d goes to the check-in reservationist.\n", guest_id);
    printf("The check-in reservationist greets Guest %d and assigns room %d.\n", guest_id, room_assigned);
    sem_post(&check_in_sem);

    // Engage in an activity
    int activity = choose_activity();
    sem_wait(&activity_sem);
    printf("Guest %d goes to the %s.\n", guest_id, activity_names[activity]);
    sleep(rand() % 3 + 1); // Simulate activity time
    activity_counters[activity]++;
    sem_post(&activity_sem);

    // Check-out process
    sem_wait(&check_out_sem);
    printf("Guest %d goes to the check-out reservationist and returns room %d.\n", guest_id, room_assigned);
    printf("The check-out reservationist greets Guest %d and receives the key for room %d.\n", guest_id, room_assigned);
    printf("Guest %d receives the receipt.\n", guest_id);
    rooms[room_assigned] = 0;
    guests_checked_out++;
    sem_post(&room_sem);
    sem_post(&check_out_sem);

    return NULL;
}

// Check-in reservationist thread function
void* check_in_reservationist(void* arg) {
    while (1) {
        sem_wait(&check_in_sem);
        // Check-in reservationist is waiting for guests
        if (guests_checked_out >= NUM_GUESTS) {
            sem_post(&check_in_sem);
            break;
        }
        sem_post(&check_in_sem);
    }
    return NULL;
}

// Check-out reservationist thread function
void* check_out_reservationist(void* arg) {
    while (1) {
        sem_wait(&check_out_sem);
        // Check-out reservationist is waiting for guests
        if (guests_checked_out >= NUM_GUESTS) {
            sem_post(&check_out_sem);
            break;
        }
        sem_post(&check_out_sem);
    }
    return NULL;
}

// Main function
int main() {
    pthread_t guests[NUM_GUESTS], check_in_thread, check_out_thread;
    int guest_ids[NUM_GUESTS];
    int i;

    // Initialize semaphores
    sem_init(&room_sem, 0, NUM_ROOMS);
    sem_init(&check_in_sem, 0, 1);
    sem_init(&check_out_sem, 0, 1);
    sem_init(&activity_sem, 0, 1);

    // Seed random number generator
    srand(time(NULL));

    // Create check-in and check-out reservationist threads
    pthread_create(&check_in_thread, NULL, check_in_reservationist, NULL);
    pthread_create(&check_out_thread, NULL, check_out_reservationist, NULL);

    // Create guest threads
    for (i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], NULL, guest, (void*)&guest_ids[i]);
    }

    // Wait for all guest threads to finish
    for (i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], NULL);
    }

    // Join check-in and check-out reservationist threads
    pthread_join(check_in_thread, NULL);
    pthread_join(check_out_thread, NULL);

    // Print activity counters
    printf("\n--------------------\n\n");
    printf("Activity Summary:\n");
    for (i = 0; i < NUM_ACTIVITIES; i++) {
        printf("%s: %d guests\n", activity_names[i], activity_counters[i]);
    }

    // Clean up
    sem_destroy(&room_sem);
    sem_destroy(&check_in_sem);
    sem_destroy(&check_out_sem);
    sem_destroy(&activity_sem);

    return 0;
}