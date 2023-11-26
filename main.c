#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define NUM_GUESTS 10
#define NUM_ROOMS 5
#define NUM_ACTIVITIES 4

// Mutex and condition variables
pthread_mutex_t room_assign_mutex, count_mutex;
pthread_cond_t check_in_cond, check_out_cond;

// Shared resources and counters
int rooms[NUM_ROOMS];
int activity_counters[NUM_ACTIVITIES] = {0};
const char* activity_names[] = {"Swimming Pool", "Restaurant", "Fitness Center", "Business Center"};

int guests_checked_out = 0; // Counter for guests who have checked out

// Function to choose a random activity for a guest
int choose_activity() {
    return rand() % NUM_ACTIVITIES;
}

// Guest thread function
void* guest(void* id) {
    int guest_id = *((int*)id);
    int assigned_room = -1;

    // Check in
    pthread_mutex_lock(&room_assign_mutex);
    while (assigned_room == -1) {
        for (int i = 0; i < NUM_ROOMS; i++) {
            if (rooms[i] == 0) { // If room is available
                rooms[i] = 1; // Assign the room
                assigned_room = i;
                printf("Guest %d enters the hotel and is assigned room %d.\n", guest_id, assigned_room);
                break;
            }
        }
        if (assigned_room == -1) {
            pthread_cond_wait(&check_in_cond, &room_assign_mutex); // Wait for room assignment
        }
    }
    pthread_mutex_unlock(&room_assign_mutex);

    // Engage in an activity
    int activity = choose_activity();
    printf("Guest %d goes to the %s.\n", guest_id, activity_names[activity]);
    sleep(rand() % 3 + 1); // Simulate activity time
    pthread_mutex_lock(&count_mutex);
    activity_counters[activity]++;
    pthread_mutex_unlock(&count_mutex);

    // Check out
    pthread_mutex_lock(&room_assign_mutex);
    printf("Guest %d checks out and leaves room %d.\n", guest_id, assigned_room);
    rooms[assigned_room] = 0; // Free the room
    guests_checked_out++;
    pthread_cond_signal(&check_out_cond); // Signal the check-out reservationist
    pthread_mutex_unlock(&room_assign_mutex);

    return NULL;
}

// Check-in reservationist thread function
void* check_in_reservationist(void* arg) {
    pthread_mutex_lock(&room_assign_mutex);
    while (guests_checked_out < NUM_GUESTS) {
        pthread_cond_signal(&check_in_cond); // Signal a waiting guest for room assignment
        pthread_cond_wait(&check_out_cond, &room_assign_mutex); // Wait for a guest to check out
    }
    pthread_mutex_unlock(&room_assign_mutex);
    return NULL;
}

// Main function
int main() {
    pthread_t guests[NUM_GUESTS], check_in_thread;
    int guest_ids[NUM_GUESTS];
    int i;

    // Initialize mutexes and condition variables
    pthread_mutex_init(&room_assign_mutex, NULL);
    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init(&check_in_cond, NULL);
    pthread_cond_init(&check_out_cond, NULL);

    // Initialize room availability and seed random number generator
    memset(rooms, 0, sizeof(rooms));
    srand(time(NULL));

    // Create check-in reservationist thread
    pthread_create(&check_in_thread, NULL, check_in_reservationist, NULL);

    // Create guest threads
    for (i = 0; i < NUM_GUESTS; i++) {
        guest_ids[i] = i;
        pthread_create(&guests[i], NULL, guest, (void*)&guest_ids[i]);
    }

    // Wait for all guest threads to finish
    for (i = 0; i < NUM_GUESTS; i++) {
        pthread_join(guests[i], NULL);
    }

    // Join the check-in reservationist thread
    pthread_join(check_in_thread, NULL);

    // Print activity counters
    printf("\n--------------------\n\n");
    printf("Activity Summary:\n");
    for (i = 0; i < NUM_ACTIVITIES; i++) {
        printf("%s: %d guests\n", activity_names[i], activity_counters[i]);
    }

    // Clean up
    pthread_mutex_destroy(&room_assign_mutex);
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&check_in_cond);
    pthread_cond_destroy(&check_out_cond);

    return 0;
}
