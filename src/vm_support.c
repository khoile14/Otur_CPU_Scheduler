/* Standard Library Includes */
#include <stdio.h>
#include <stdlib.h>
/* Linux System API Includes */
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
/* StrawHat Function Includes */
#include "vm.h"
#include "vm_cs.h"
#include "vm_settings.h"
#include "vm_shell.h"
#include "vm_printing.h"
#include "vm_support.h"

static int is_high(Otur_process_s *node) {
  return(node && ((node->state >> 15)&1));
}
static int is_running(Otur_process_s *node) {
  return(node && ((node->state >> 14)&1));
}
static int is_ready(Otur_process_s *node) {
  return(node && ((node->state >> 13)&1));
}
static int is_defunct(Otur_process_s *node) {
  return(node && ((node->state >> 12)&1));
}
static int is_critical(Otur_process_s *node) {
  return(node && ((node->state >> 11)&1));
}
static int get_ec(Otur_process_s *node) {
  if(!node) {
    return -1;
  }
  return (node->state & ((1 << 8)-1));
}

/* Quickly registers a new signal with the given signal number and handler
 * Exits the program on errors with signal registration.
 */
void register_signal(int sig, void (*handler)(int)) {
  if(handler == NULL) {
    ABORT_ERROR("Handler Function Needed for Registration");
  }

  // Register the handler with the OS
  struct sigaction sa = {0};
  sa.sa_handler = handler;
  sigaction(sig, &sa, NULL);
}

/* Print the Virtual System Prompt
 * PROMPT configurable in inc/vm_settings.h
 */
void print_prompt() {
  printf("%s(PID: %d)%s %s%s%s ", BLUE, getpid(), RST, GREEN, PROMPT, RST);
  fflush(stdout);
}

/* Special Error that also immediately exits the program. */
void abort_error(char *msg, char *src) {
  fprintf(stderr, "  %s[ERROR ] %s (File %s)%s\n", RED, msg, src, RST);
  fprintf(stderr, "  %s         Terminating Program%s\n", RED, RST);
  exit(EXIT_FAILURE);
}

/* Prints out the Opening Banner */
void print_strawHat_banner() {
  printf(
" %sGreen Imperial HEx:%s %sStrawHat Task Manager v1.5a%s %s*TRIAL EXPIRED*%s\n%s"
"               [o]                     \n"
"     .__________|__________.           \n"
"     |%s        _.--._       %s|			  \n"
"     |%s   _.-.'      `.-.   %s|			  \n"
"     |%s .' .%s/`--...--' \\%s `. %s|  \n"
"     |%s `.'.%s`--.._..--'%s  .' %s|	  \n"
"     |%s   ' -.._______..'   %s|	      \n"
"     |%s                     %s|       \n"
"     |%s %s[StrawHat-VM] $%s _   |     \n"
"     ._____________________.           \n"
"               [  ]                    \n"
"       ____________________            \n"
"    _-'.-.-.-.-. .-.-.-.-. `-_    \n" // Keyboard and Hat Derived from Dan Strychalski's Work
"  _'.-.-.--------------.-.-.-.`-_ \n" // Original Keyboard at https://www.asciiart.eu/computers/keyboards
" :-------------------------------:%s\n", // Original Hat at https://www.asciiart.eu/clothing-and-accessories/hats
  GREEN, RST, BLUE, RST, RED, RST, WHITE,
  YELLOW, WHITE,
  YELLOW, WHITE,
  YELLOW, RED, YELLOW, WHITE,
  YELLOW, RED, YELLOW, WHITE,
  YELLOW, WHITE,
  YELLOW, WHITE,
  YELLOW, GREEN, WHITE,
  RST);
}

/* Prints the full Schedule of all processes being tracked. */
void print_otur_debug(Otur_schedule_s *schedule, Otur_process_s *on_cpu) {
  // If on_cpu is NULL, it won't be printed, but otherwise is legal
  
  // Only print if debug mode is enabled.
  if(g_debug_mode == 1) {
    print_schedule(schedule, on_cpu);
  }
}

/* Prints the full Schedule of all processes being tracked.
 * If on_cpu is NULL, simply won't print that anything is on CPU, but is legal */
void print_schedule(Otur_schedule_s *schedule, Otur_process_s *on_cpu) {
  // If no such schedule exists, print nothing.
  if(schedule == NULL) {
    return;
  }

  // Collect the number of processes in StrawHat
  int rqh_count = otur_count(schedule->ready_queue_high);
  int rqn_count = otur_count(schedule->ready_queue_normal);
  int dq_count = otur_count(schedule->defunct_queue);

  if(rqh_count == -1 || rqn_count == -1 || dq_count == -1) {
    ABORT_ERROR("otur_count returned an Error Condition.");
  }

  int total_scheduled_processes = rqh_count + rqn_count + dq_count;
  PRINT_STATUS("Printing the current Status...");
  PRINT_STATUS("Running Process (Note: Processes run briefly, so this is usually empty.)");

  // CPU - Add the On CPU Process to the count
  int count = 0;
  if(on_cpu != NULL) {
    count = 1;
  }

  // Now print all the information
  PRINT_STATUS("...[CPU Execution    - %2d Process%s]", count, count?"":"es");
  print_process_node(on_cpu);
  
  // Schedule
  PRINT_STATUS("Schedule - %d Processes across all Queues", total_scheduled_processes);
  // Ready Queue - High Pri
  count = otur_count(schedule->ready_queue_high);
  if(count == -1) {
    ABORT_ERROR("otur_count returned an Error Condition.");
  }
  PRINT_STATUS("...[Ready Queue - High   - %2d Process%s]", count, count==1?"":"es");
  print_otur_queue(schedule->ready_queue_high);
  // Ready Queue - Normal
  count = otur_count(schedule->ready_queue_normal);
  if(count == -1) {
    ABORT_ERROR("otur_count returned an Error Condition.");
  }
  PRINT_STATUS("...[Ready Queue - Normal - %2d Process%s]", count, count==1?"":"es");
  print_otur_queue(schedule->ready_queue_normal);
  // Defunct Queue
  count = otur_count(schedule->defunct_queue);
  if(count == -1) {
    ABORT_ERROR("otur_count returned an Error Condition.");
  }
  PRINT_STATUS("...[Defunct Queue        - %2d Process%s]", count, count==1?"":"es");
  print_otur_queue(schedule->defunct_queue);
}

/* Prints a single Scheduler Queue */
void print_otur_queue(Otur_queue_s *queue) {
  // If no such queue exists, print nothing.
  if(queue == NULL) {
    return;
  }

  // Iterate the queue and print each process
  Otur_process_s *walker = queue->head;
  while(walker != NULL) {
    print_process_node(walker);
    walker = walker->next;
  }
}

// Prints a schedule tracked process
void print_process_node(Otur_process_s *node) {
  // If no process exists, print nothing.
  if(node == NULL) {
    return;
  }
  // If Process has Terminated
  if(is_defunct(node)) {
    PRINT_STATUS("     [PID: %7d] %-26s ... Flags: [%c%c%c%c%c], Age: %2d, Exit Code: %d",
        node->pid, node->cmd,  
        is_high(node)?    'H':' ',
        is_running(node)? 'U':' ',
        is_ready(node)?   'R':' ',
        is_defunct(node)? 'D':' ',
        is_critical(node)?'C':' ',
        node->age,
        get_ec(node));
  }
  // If Process has not Terminated Yet
  else {
    PRINT_STATUS("     [PID: %7d] %-26s ... Flags: [%c%c%c%c%c], Age: %2d",
        node->pid, node->cmd,  
        is_high(node)?    'H':' ',
        is_running(node)? 'U':' ',
        is_ready(node)?   'R':' ',
        is_defunct(node)? 'D':' ',
        is_critical(node)?'C':' ',
        node->age);
  }
}
