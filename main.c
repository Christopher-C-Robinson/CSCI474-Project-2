#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_GUESTS 10
#define NUM_ROOMS 5
#define NUM_RESERVATIONISTS 2

// Semaphores
sem_t room_sem[NUM_ROOMS];
sem_t check_in_queue_sem, check_out_queue_sem;
sem_t check_in_process_sem[NUM_RESERVATIONISTS];
sem_t check_out_process_sem[NUM_RESERVATIONISTS];
sem_t checked_in_sem[NUM_GUESTS];
sem_t checked_out_sem[NUM_GUESTS];
sem_t room_available_sem;

// Shared resources
int rooms[NUM_ROOMS] = {0}; // Room availability
int activity_counter[4] = {0}; // Counter for each activity
int check_in_queue[NUM_GUESTS], check_out_queue[NUM_GUESTS];
int check_in_queue_size = 0, check_out_queue_size = 0;

// Thread functions
void* guest(void* arg);
void* check_in_reservationist(void* arg);
void* check_out_reservationist(void* arg);
void enqueue(int queue[], int *queue_size, int value);
int dequeue(int queue[], int *queue_size);

int main() {
    pthread_t guests[NUM_GUESTS];
    pthread_t check_in_threads[NUM_RESERVATIONISTS], check_out_threads[NUM_RESERVATIONISTS];
    int i;

    // Initialize semaphores
    for (i = 0; i < NUM_ROOMS; i++) {
        sem_init(&room_sem[i], 0, 1); // Room semaphores
    }
    sem_init(&check_in_queue_sem, 0, 1); // Queue semaphores
    sem_init(&check_out_queue_sem, 0, 1);
    sem_init(&room_available_sem, 0, 0); // Room availability semaphore
    for (i = 0; i < NUM_RESERVATIONISTS; i++) {
        sem_init(&check_in_process_sem[i], 0, 1); // Process semaphores
        sem_init(&check_out_process_sem[i], 0, 1);
    }
    for (i = 0; i < NUM_GUESTS; i++) {
        sem_init(&checked_in_sem[i], 0, 0); // Guest check-in/out semaphores
        sem_init(&checked_out_sem[i], 0, 0);
    }

    // Create reservationist threads
    for (i = 0; i < NUM_RESERVATIONISTS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&check_in_threads[i], NULL, check_in_reservationist, id);
        pthread_create(&check_out_threads[i], NULL, check_out_reservationist, id);
    }

    // Create guest threads
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
    sem_destroy(&check_in_queue_sem);
    sem_destroy(&check_out_queue_sem);
    sem_destroy(&room_available_sem);
    for (i = 0; i < NUM_RESERVATIONISTS; i++) {
        sem_destroy(&check_in_process_sem[i]);
        sem_destroy(&check_out_process_sem[i]);
    }
    for (i = 0; i < NUM_GUESTS; i++) {
        sem_destroy(&checked_in_sem[i]);
        sem_destroy(&checked_out_sem[i]);
    }

    return 0;
}

void enqueue(int queue[], int *queue_size, int value) {
    queue[*queue_size] = value;
    (*queue_size)++;
}

int dequeue(int queue[], int *queue_size) {
    int value = queue[0];
    for (int i = 0; i < *queue_size - 1; i++) {
        queue[i] = queue[i + 1];
    }
    (*queue_size)--;
    return value;
}

void* guest(void* arg) {
    int id = *(int*)arg;
    char* activities[] = {"swimming pool", "restaurant", "fitness center", "business center"};
    int activity_index;

    printf("Guest %d enters the hotel\n", id);
    sem_wait(&check_in_queue_sem);
    enqueue(check_in_queue, &check_in_queue_size, id);
    sem_post(&check_in_queue_sem);

    // Notify one of the check-in reservationists
    for (int i = 0; i < NUM_RESERVATIONISTS; i++) {
        sem_post(&check_in_process_sem[i]);
    }

    // Wait for room assignment
    sem_wait(&checked_in_sem[id]);

    // Hotel activity
    activity_index = rand() % 4;
    printf("Guest %d goes to the %s\n", id, activities[activity_index]);
    activity_counter[activity_index]++;
    sleep(rand() % 3 + 1);

    sem_wait(&check_out_queue_sem);
    enqueue(check_out_queue, &check_out_queue_size, id);
    sem_post(&check_out_queue_sem);

    // Notify one of the check-out reservationists
    for (int i = 0; i < NUM_RESERVATIONISTS; i++) {
        sem_post(&check_out_process_sem[i]);
    }

    // Wait for check-out completion
    sem_wait(&checked_out_sem[id]);

    free(arg);
    return NULL;
}

void* check_in_reservationist(void* arg) {
    int res_id = *(int*)arg;
    while (1) {
        sem_wait(&check_in_process_sem[res_id]); // Wait for a guest to be ready to check in

        sem_wait(&check_in_queue_sem);
        if (check_in_queue_size > 0) {
            int guest_id = dequeue(check_in_queue, &check_in_queue_size);
            sem_post(&check_in_queue_sem);

            int room_assigned = -1;
            while (room_assigned == -1) {
                for (int i = 0; i < NUM_ROOMS; i++) {
                    sem_wait(&room_sem[i]); // Try to acquire the room semaphore
                    if (!rooms[i]) { // Check if room is unoccupied
                        rooms[i] = 1; // Mark room as occupied
                        room_assigned = i;
                        sem_post(&room_sem[i]);
                        break;
                    } else {
                        sem_post(&room_sem[i]); // Release semaphore if room is occupied
                    }
                }

                if (room_assigned == -1) {
                    // No room available, wait for a room to become available
                    sem_wait(&room_available_sem);
                }
            }

            printf("Check-in reservationist %d greets Guest %d and assigns Room %d\n", res_id, guest_id, room_assigned);
            sem_post(&checked_in_sem[guest_id]); // Signal the guest that check-in is complete
        } else {
            sem_post(&check_in_queue_sem);
        }
    }
    free(arg);
    return NULL;
}

void* check_out_reservationist(void* arg) {
    int res_id = *(int*)arg;
    while (1) {
        sem_wait(&check_out_process_sem[res_id]);
        sem_wait(&check_out_queue_sem);
        if (check_out_queue_size > 0) {
            int guest_id = dequeue(check_out_queue, &check_out_queue_size);
            sem_post(&check_out_queue_sem);

            int room_vacated = -1;
            for (int i = 0; i < NUM_ROOMS; i++) {
                sem_wait(&room_sem[i]);
                if (rooms[i]) {
                    rooms[i] = 0; // Vacate the room
                    room_vacated = i;
                    sem_post(&room_sem[i]);
                    break;
                }
                sem_post(&room_sem[i]);
            }

            printf("Check-out reservationist %d greets Guest %d\n", res_id, guest_id);
            printf("Check-out reservationist %d printed the receipt for Guest %d\n", res_id, guest_id);
            sem_post(&checked_out_sem[guest_id]);

            if (room_vacated != -1) {
                sem_post(&room_available_sem); // Signal that a room is now available
            }
        } else {
            sem_post(&check_out_queue_sem);
        }
    }
    free(arg);
    return NULL;
}
