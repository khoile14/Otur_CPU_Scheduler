/* DO NOT EDIT THIS FILE
 * - otur_sched.h (Part of the Otur Scheduler)
 * - Copyright: Prof. Kevin Andrea, George Mason University.  All Rights Reserved
 * - Date: Aug 2024
 *
 *   Definitions for the Otur Scheduler System
 *   (Dependency - StrawHat VM Settings)
 */

#ifndef OTUR_SCHED_H
#define OTUR_SCHED_H

#include "vm_settings.h"

// Process Node Definition
typedef struct process_node {
  pid_t pid;            // PID of the Process you're Tracking
  char *cmd;            // Name of the Process being run
  unsigned short state; // 16-bit: Contains the Flags [H,U,R,D,C] AND Exit Code
  int age;              // How long this has been in the Ready Queue - Low since last run
  struct process_node *next; // Pointer to next Process Node in a linked list
} Otur_process_s;

// Queue Header Definition
typedef struct queue_header {
  int count;            // How many Nodes are in this linked list?
  Otur_process_s *head; // Points to FIRST node of linked list.  No Dummy Nodes.
  Otur_process_s *tail; // Points to LAST node of linked list.  No Dummy Nodes. Optional.
} Otur_queue_s;

// Schedule Header Definition
typedef struct otur_schedule {
  Otur_queue_s *ready_queue_high; // Linked List of High Priority Processes ready to Run on CPU
  Otur_queue_s *ready_queue_normal; // Linked List of Normal Processes ready to Run on CPU
  Otur_queue_s *defunct_queue; // Linked List of Defunct Processes 
} Otur_schedule_s;

// Prototypes
Otur_schedule_s *otur_initialize();
Otur_process_s *otur_invoke(pid_t pid, int is_high, int is_critical, char *command);
int otur_enqueue(Otur_schedule_s *schedule, Otur_process_s *process);
int otur_count(Otur_queue_s *queue);
Otur_process_s *otur_select(Otur_schedule_s *schedule);
int otur_promote(Otur_schedule_s *schedule);
int otur_exited(Otur_schedule_s *schedule, Otur_process_s *process, int exit_code);
int otur_killed(Otur_schedule_s *schedule, pid_t pid, int exit_code);
int otur_reap(Otur_schedule_s *schedule, pid_t pid);
void otur_cleanup(Otur_schedule_s *schedule);

#endif
