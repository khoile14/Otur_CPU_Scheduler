/* Standard Library Includes */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
/* Unix System Includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
/* Local Includes */
#include "otur_sched.h"
#include "vm_support.h"
#include "vm_process.h"

/* Feel free to create any helper functions you like! */

/*** Otur Library API Functions to Complete ***/

/* Initializes the Otur_schedule_s Struct and all of the Otur_queue_s Structs
 * Follow the project documentation for this function.
 * Returns a pointer to the new Otur_schedule_s or NULL on any error.
 * - Hint: What does malloc return on an error?
 */
Otur_schedule_s *otur_initialize() {
    Otur_schedule_s *schedule = malloc(sizeof(Otur_schedule_s)); /* Initialize schedule */
    Otur_process_s *temp = NULL;
    if (schedule == NULL) { /* check if the initialization is fail then return null */

        return NULL;
    }


    schedule->ready_queue_normal = malloc(sizeof(Otur_queue_s)); /* initialize normal queue */
    if (schedule->ready_queue_normal == NULL) { /* check if the initialization is fail then return null */
        free(schedule); /* free the schedule if it is fail*/
        return NULL;
    }


    schedule->ready_queue_high = malloc(sizeof(Otur_queue_s)); /*Initialize high queue */
    if (schedule->ready_queue_high == NULL) {
        temp = schedule->ready_queue_normal->head;
        while (temp!= NULL){ /* if the initialization fail then free everything from normal queu as well as the schedule */
            temp = schedule->ready_queue_normal->head;
            schedule->ready_queue_normal->head = temp->next;
            free(temp->cmd);
            free(temp);
        }
        free(schedule->ready_queue_normal);
        free(schedule);
        return NULL;
    }


    schedule->defunct_queue = malloc(sizeof(Otur_queue_s)); /*Initialize defunct queue */
    if (schedule->defunct_queue == NULL) {
        /*if the initialization fail then free everything from both normal and high queue */
        temp = schedule->ready_queue_normal->head;
        while (temp!= NULL){
            temp = schedule->ready_queue_normal->head;
            schedule->ready_queue_normal->head = temp->next;
            free(temp->cmd);
            free(temp);
        }
        free(schedule->ready_queue_normal);
        temp = schedule->ready_queue_high->head;
        while (temp!= NULL){
            temp = schedule->ready_queue_high->head;
            schedule->ready_queue_high->head = temp->next;
            free(temp->cmd);
            free(temp);
        }
        free(schedule->ready_queue_high);
        free(schedule);
        return NULL;
    }

    /* set all of the head and tail as well as count of each queue to proper values */
    schedule->ready_queue_normal->head = NULL;
    schedule->ready_queue_normal->tail = NULL;
    schedule->ready_queue_normal->count = 0;


    schedule->ready_queue_high->head = NULL;
    schedule->ready_queue_high->tail = NULL;
    schedule->ready_queue_high->count = 0;


    schedule->defunct_queue->head = NULL;
    schedule->defunct_queue->tail = NULL;
    schedule->defunct_queue->count = 0;


    return schedule;
}


/* Allocate and Initialize a new Otur_process_s with the given information.
 * - Malloc and copy the command string, don't just assign it!
 * Follow the project documentation for this function.
 * - You may assume all arguments are Legal and Correct for this Function Only
 * Returns a pointer to the Otur_process_s on success or a NULL on any error.
 */
Otur_process_s *otur_invoke(pid_t pid, int is_high, int is_critical, char *command) {
    Otur_process_s *process = malloc(sizeof(Otur_process_s)); /*allocating memory for the process */
    if (process == NULL) {
        return NULL;
    }


    process->pid = pid; /* set the pid of the process to the input pid */


    process->state |= (1 << 13); /* set the state of the process to ready state by shifting it left 13 spot */


    if (is_critical != 0) { /* check if critial is true then set it to 1 else set it to 0 */
        process->state |= (1 << 11);
    } else {
        process->state &= ~(1 << 11);
    }


    if (is_high != 0 || is_critical != 0) { /* check if high or critical is true then set high bit to 1 else set it to 0 */
        process->state |= (1 << 15);
    } else {
        process->state &= ~(1 << 15);
    }


    process->state = ((process->state >> 8) << 8); /* set all the lower 8 bits to 0 */
    process->age = 0; /* set the age to 0 */
    process->cmd = malloc(sizeof(char) * (strlen(command) + 1)); /* allocating memory for cmd */
    if(process->cmd == NULL) { /* if the allocation fail then free it and return null */
        free(process);
        return NULL;
    }
    strncpy(process->cmd, command, strlen(command) + 1); /* copy the command to cmd using strncpy for security */
    process->next = NULL;


    return process;
}


/* Inserts a process into the appropriate Ready Queue (singly linked lists).
 * Follow the project documentation for this function.
 * - Do not create a new process to insert, insert the SAME process passed in.
 * Returns a 0 on success or a -1 on any error.
 */
void add_to_queue(Otur_queue_s *queue, Otur_process_s *process) { /* helper function that add a process into a queue */
    if (queue->head == NULL) {
        queue->head = process;
        queue->tail = process;
    } else {
        queue->tail->next = process;
        queue->tail = process;
    }
    queue->count++; /* increment the count after add */
}


int otur_enqueue(Otur_schedule_s *schedule, Otur_process_s *process) {
    if (process == NULL || schedule == NULL) {
        return -1;
    }
    process->state |= 0x7000; /* set all 3 state flags to be 1 */
    process->state ^= 0x5000; /* use xor to make running and defunct to be 0 */


    if (process->state & ((1 << 15) | (1 << 11))) { /*check if high or critical is 1 then insert in it to reaady high */
        add_to_queue(schedule->ready_queue_high, process);
    } else {
        add_to_queue(schedule->ready_queue_normal, process); /* if not then insert in ready normal */

    }
    return 0;
}


/* Returns the number of items in a given Otur Queue (singly linked list).
 * Follow the project documentation for this function.
 * Returns the number of processes in the list or -1 on any errors.
 */
int otur_count(Otur_queue_s *queue) {
    if (queue == NULL) {
        return -1;
    }
    return queue->count; /* return the count of the input queue */
}

/* helper remove function that remove a process from input queue */
Otur_process_s *remove_function(Otur_queue_s *queue, Otur_process_s *process) {
    Otur_process_s *temp = queue->head;
    Otur_process_s *temp2 = NULL;
    if (temp == NULL) {
        return NULL;
    }
    if (temp == process) {
        queue->head = temp->next;
        queue->count--;
        temp->next = NULL;
        return temp;
    }
    while (temp->next != NULL) {
        if (temp->next == process) {
            queue->count--;
            temp2 = temp->next;
            temp->next = temp->next->next;
            temp2->next = NULL;
            return temp2;
        }
        temp = temp->next;
    }
    return NULL;
}


/* Selects the best process to run from the Ready Queue (singly linked list).
 * Follow the project documentation for this function.
 * Returns a pointer to the process selected or NULL if none available or on any errors.
 * - Do not create a new process to return, return a pointer to the SAME process selected.
 */
Otur_process_s *otur_select(Otur_schedule_s *schedule) {
    Otur_process_s *temp = NULL;
    Otur_process_s *temp2 = NULL;


    if (schedule == NULL) {
        return NULL;
    }


    if (schedule->ready_queue_high != NULL) {
        temp = schedule->ready_queue_high->head; /* set the temp to the head of queue high */

        while (temp != NULL) { /* loop through the queue */
            if (temp->state & (1 << 13)) { /* check if there are critical processes */
                temp2 = remove_function(schedule->ready_queue_high, temp); /* remove it from the queue */
                temp2->age = 0; /* set its age to 0 */
                temp2->state |= 0x2000; /* set the running state to 1 */
                temp2->next = NULL; /* set the pointer to next node to be null */
                return temp2; /* return the node */
            }
            temp = temp->next;
        }
        if (schedule->ready_queue_high->head != NULL) { /* do the same as the above but only remove the head */
            temp2 = remove_function(schedule->ready_queue_high, schedule->ready_queue_high->head);
            if (temp2 != NULL) {
                temp2->age = 0;
                temp2->state |= 0x2000;
                temp2->next = NULL;
                return temp2;
            }
        }
    } else if (schedule->ready_queue_normal != NULL) { /* if there are none then do the same in normal queue */
        temp2 = remove_function(schedule->ready_queue_normal, schedule->ready_queue_normal->head);
        if (temp2 != NULL) {
            temp2->age = 0;
            temp2->state |= 0x2000;
            temp2->next = NULL;
            return temp2;
        }
    }
    return NULL; /* return null if both are empty */
}


/* Ages up all Process nodes in the Ready Queue - Normal and Promotes any that are Starving.
 * - If the Ready Queue - Normal is empty, return 0.  (Nothing to do, so it was a success)
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */


int otur_promote(Otur_schedule_s *schedule) {
    if (schedule == NULL || schedule->ready_queue_normal == NULL || schedule->ready_queue_high == NULL) {
        return -1;
    }
    Otur_process_s *temp = schedule->ready_queue_normal->head; /* set the temp to the head of normal queue */
    Otur_process_s *temp2 = NULL;
    if (temp == NULL) {
        return 0;
    }
    while (temp != NULL) {
        temp->age++; /* increase the age by 1 */
        Otur_process_s *next = temp->next; /* create a pointer that keep track of the node ahead */
        if (temp->age >= STARVING_AGE) { /* check if the current node is greater or equal to starving age*/
            temp2 = remove_function(schedule->ready_queue_normal, temp); /* remove the current node from the queue */
            temp2->next = NULL;
            add_to_queue(schedule->ready_queue_high, temp2); /* add it to queue high */
        }
        temp = next; /* set it to the pointer ahead of it to continue the search */
    }
    return 0;
}

/* This is called when a process exits normally that was just Running.
 * Put the given node into the Defunct Queue and set the Exit Code into its state
 * - Do not create a new process to insert, insert the SAME process passed in.
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int otur_exited(Otur_schedule_s *schedule, Otur_process_s *process, int exit_code) {
    if (schedule == NULL || process == NULL) {
        return -1;
    }
    process->state |= (1 << 12); /* set the defunct to 1 */
    process->state &= ~0xFF;/* set the lower 8 bits to 0 */

    process->state |= (exit_code & 0xFF); /* Set the state bits used for exit_code to the value of the exit_code passed in.*/

    if (schedule->defunct_queue->head == NULL) { /* insert it at the end of defunct queue */
        schedule->defunct_queue->head = process;
        schedule->defunct_queue->tail = process;
    } else {
        schedule->defunct_queue->tail->next = process;
        schedule->defunct_queue->tail = process;
    }
    schedule->defunct_queue->count++;
    return 0;
}

/* This is called when StrawHat kills a process early (kill command).
 * - The Process will either be in your Ready or Suspended Queue, or neither.
 * - The difference with otur_exited is that this process is in one of your Queues already.
 * Remove the process with matching pid from the Ready or Suspended Queue and add the Exit Code to it.
 * - You have to check both since it could be in either queue.
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error (eg. process not found).
 */
Otur_process_s *remove_from_queue(Otur_queue_s *queue, pid_t pid) { /* helper remove function that take input of queue and pid number */
    Otur_process_s *temp = queue->head;
    Otur_process_s *prev = NULL;

    while (temp != NULL) {
        if (temp->pid == pid) {
            if (prev == NULL) {
                queue->head = temp->next;
            } else {
                prev->next = temp->next;
            }
            queue->count--;
            temp->next = NULL;
            return temp;
        }
        prev = temp;
        temp = temp->next;
    }
    return NULL;
}

int otur_killed(Otur_schedule_s *schedule, pid_t pid, int exit_code) {
    if (schedule == NULL) {
        return -1;
    }

    Otur_process_s *process = remove_from_queue(schedule->ready_queue_high, pid); /* try to remove a process with specific pid from queue high */
    if (process == NULL) { /* if there is none then remove it from the normal queue instead */
        process = remove_from_queue(schedule->ready_queue_normal, pid);
    }

    if (process == NULL) { /* if there is none in both then return -1 */
        return -1;
    }

    process->state |= 0x7000; /* set all 3 flags to 1 */
    process->state ^= 0x6000; /* use xor to set defunct to 1 */

    process->state &= ~0xFF; /* set the lower 8 bits to 0 */
    process->state |= (exit_code & 0xFF); /*Set the state bits used for exit_code to the value of the exit_code passed in*/

    add_to_queue(schedule->defunct_queue, process); /* add it in to the defunct queue */

    return 0;
}

/* This is called when the StrawHat reaps a Defunct process. (reap command)
 * Remove and free the process with matching pid from the Defunct Queue and return its exit code.
 * Follow the project documentation for this function.
 * Returns the process' exit code on success or a -1 if no such process or on any error.
 */

int otur_reap(Otur_schedule_s *schedule, pid_t pid) {
    if (schedule == NULL) {
        return -1;
    }

    Otur_process_s *temp = schedule->defunct_queue->head; /* set the temp to head of defunct queue */
    unsigned short exit_code;

    if (pid == 0) { /* check if the pid parameter is 0 then remove the head */
        if (temp == NULL) {
            return -1;
        }
        schedule->defunct_queue->head = temp->next;
        temp->next = NULL;
        exit_code = ((temp->state) &=(0x00FF));
        schedule->defunct_queue->count--;
        free(temp -> cmd);
        free(temp);
        return exit_code;
    }else{
        Otur_process_s  *node = remove_from_queue(schedule->defunct_queue, pid); /* if not then remove the node with the pid */
        if (node == NULL) {
            return -1;
        }
        exit_code = ((node->state) &=(0x00FF)); /* get the exit code */
        free(node -> cmd); /* free the node cmd */
        free(node); /* free the node */
        return exit_code; /* return the exit code */
    }
}



/* Frees all allocated memory in the Otur_schedule_s, all of the Queues, and all of their Nodes.
 * Follow the project documentation for this function.
 * Returns void.
 */
void otur_cleanup(Otur_schedule_s *schedule) {
    Otur_process_s *temp = NULL;
    Otur_process_s *temp2 = NULL;

    /*Free the nodes in the ready_queue_high*/
    temp = schedule->ready_queue_high->head;
    while (temp != NULL) {
        temp2 = temp;
        temp = temp->next;
        free(temp2->cmd);
        free(temp2);
    }
    free(schedule->ready_queue_high);

    /* Free the nodes in the ready_queue_normal */
    temp = schedule->ready_queue_normal->head;
    while (temp != NULL) {
        temp2 = temp;
        temp = temp->next;
        free(temp2->cmd);
        free(temp2);
    }
    free(schedule->ready_queue_normal);

    /* Free the nodes in the defunct_queue */
    temp = schedule->defunct_queue->head;
    while (temp != NULL) {
        temp2 = temp;
        temp = temp->next;
        free(temp2->cmd);
        free(temp2);
    }
    free(schedule->defunct_queue);

    /* Finally, free the schedule itself */
    free(schedule);
}
