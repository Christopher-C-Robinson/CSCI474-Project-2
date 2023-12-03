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
sem_t check_in_ready_sem;
sem_t check_out_ready_sem;

// Shared data
int available_rooms[NUM_ROOMS] = {0, 1, 2};
int activity_count[4] = {0};
int checked_in[NUM_GUESTS] = {0}; // Array to keep track of which guest has checked in

// Randomly choose an activity for guests
const char *activities[] = {"swimming pool", "restaurant", "fitness center", "business center"};

// Guest thread routine
void *guest_routine(void *arg) {
    int guest_id = *(int *)arg;

    // Try to enter the hotel and check in
    sem_wait(&room_sem);
    printf("Guest %d enters the hotel.\n", guest_id);

    // Check-in process
    sem_post(&check_in_ready_sem); // Signal that the guest is ready to check in
    sem_wait(&check_in_sem); // Wait for the check-in reservationist to complete the check-in

    // Wait until the room is assigned (checked_in[guest_id] is set)
    while (!checked_in[guest_id]) {
        sched_yield(); // Yield the processor to other threads
    }

    // Guest activity
    int activity_index = rand() % 4;
    printf("Guest %d goes to the %s.\n", guest_id, activities[activity_index]);
    activity_count[activity_index]++;
    sleep(rand() % 3 + 1);

    // Check-out process
    sem_post(&check_out_ready_sem); // Signal that the guest is ready to check out
    sem_wait(&check_out_sem); // Wait for the check-out reservationist to complete the check-out
    printf("Guest %d receives the receipt and leaves.\n", guest_id);

    // Return the room key and mark as checked out
    for (int i = 0; i < NUM_ROOMS; i++) {
        if (checked_in[i] == guest_id) { // Find the room assigned to this guest
            available_rooms[i] = i; // Mark room as available again
            checked_in[i] = 0; // Mark as checked out
            break;
        }
    }

    sem_post(&room_sem); // Indicate that the room is now available

    return NULL;
}

// Check-in reservationist routine
void *check_in_routine(void *arg) {
    for (int i = 0; i < NUM_GUESTS; i++) {
        sem_wait(&check_in_ready_sem); // Wait for a guest to be ready to check in

        // Greet guest and assign room
        sem_wait(&room_sem); // Acquire room semaphore for the duration of the check-in process
        int room_assigned = 0;
        for (int j = 0; j < NUM_ROOMS; j++) {
            if (available_rooms[j] != -1) {
                printf("The check-in reservationist greets Guest %d.\n", i);
                printf("Check-in reservationist assigns Room %d to Guest %d.\n", j, i);
                available_rooms[j] = -1; // Mark room as occupied
                checked_in[i] = 1; // Mark as checked in
                room_assigned = 1;
                break;
            }
        }
        sem_post(&room_sem); // Release room semaphore after room assignment

        if (room_assigned) {
            sem_post(&check_in_sem); // Signal the guest thread that check-in is complete
        } else {
            // If no room was available, we need to cycle again until a room is free
            i--;
            sleep(1); // Sleep briefly to give up CPU and allow guests to check out
        }
    }
    return NULL;
}

void *check_out_routine(void *arg) {
    for (int i = 0; i < NUM_GUESTS; i++) {
        sem_wait(&check_out_ready_sem); // Wait for a guest to be ready to check out

        // Process the check-out
        printf("The check-out reservationist processes the check-out for Guest %d.\n", i);

        // Find the room associated with the guest and mark it as available
        for (int j = 0; j < NUM_ROOMS; j++) {
            if (available_rooms[j] == -1) {
                available_rooms[j] = j; // Mark room as available
                break;
            }
        }

        printf("The receipt was printed for Guest %d.\n", i);
        sem_post(&check_out_sem); // Signal the guest thread
        sem_post(&room_sem); // Increment the room semaphore
    }
    return NULL;
}

int main()
{
    srand(time(NULL)); // Seed the random number generator

    // Initialize semaphores
    sem_init(&room_sem, 0, NUM_ROOMS);
    sem_init(&check_in_sem, 0, 1);
    sem_init(&check_out_sem, 0, 1);
    sem_init(&check_in_ready_sem, 0, 0);  // Initialize to 0 because no guest is ready at the start
    sem_init(&check_out_ready_sem, 0, 0); // Initialize to 0 because no guest is ready at the start

    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_res;
    pthread_t check_out_res;
    int guest_ids[NUM_GUESTS];

    // Create guest threads
    for (int i = 0; i < NUM_GUESTS; i++)
    {
        guest_ids[i] = i;
        pthread_create(&guests[i], NULL, guest_routine, &guest_ids[i]);
    }

    // Create check-in and check-out reservationist threads
    pthread_create(&check_in_res, NULL, check_in_routine, NULL);
    pthread_create(&check_out_res, NULL, check_out_routine, NULL);

    // Join guest threads
    for (int i = 0; i < NUM_GUESTS; i++)
    {
        pthread_join(guests[i], NULL);
    }

    // Terminate reservationist threads
    pthread_cancel(check_in_res);
    pthread_cancel(check_out_res);

    // Destroy semaphores
    sem_destroy(&room_sem);
    sem_destroy(&check_in_sem);
    sem_destroy(&check_out_sem);
    sem_destroy(&check_in_ready_sem);
    sem_destroy(&check_out_ready_sem);

    // Accounting
    printf("\nTotal Guests: %d\n", NUM_GUESTS);
    printf("Pool: %d\n", activity_count[0]);
    printf("Restaurant: %d\n", activity_count[1]);
    printf("Fitness Center: %d\n", activity_count[2]);
    printf("Business Center: %d\n", activity_count[3]);

    return 0;
}
