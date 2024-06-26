/* Standard Library Includes */
#include <stdio.h>
#include <stdlib.h>
/* Linux System API Includes */
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
/* Project Includes */
#include "vm_support.h"
#include "vm_shell.h"
#include "vm_process.h"
#include "vm_printing.h"
#include "vm_cs.h"

/* Project Globals */
int g_debug_mode = DEFAULT_DEBUG; // Default is to start at Debug OFF.
// If you want to change the default debug start, the define is in inc/vm_settings.h

/* Handler for Segmentation Faults */
void hnd_sigsegv(int sig) {
  // ABORT_ERROR exits the program immediately.
  ABORT_ERROR("Segmentation Fault Detected!\n");
}

/* Handler for Ctrl-C entered by the User (Toggle the CS System) */
void hnd_sigint(int sig) {
  // This is faster than using 'start' and 'stop'
  handle_ctrlc(); // Toggles the Context Switch System on and off. 
}

/* Cleans up all threads and created processes.
 * - Registered with atexit
 */
void vm_cleanup() {
  PRINT_STATUS("Cleaning up SHVM environment.");
  cs_cleanup();  // Shuts down and cleans up the CS system fully.

  PRINT_STATUS("Deallocating all Processes.");
  deallocate_process_system();
}

/* Set up the main VM environment, then drop to a user shell.
 * Returns 0 on Succesful completion of the program.
 */
int main() {
  // Registers functions to be called on Ctrl-C (SIGINT) or Segfault
  register_signal(SIGSEGV, hnd_sigsegv);
  register_signal(SIGINT, hnd_sigint);

  // Print our nice intro banner art!
  // - Art is defined in vm_support.c
  print_strawHat_banner();

  // Registers a function to be called on exit.  
  atexit(vm_cleanup);

  // Begin Running the Context Switch Threading System
  initialize_cs_system();

  // Set up main VM Environment to handle and track Jobs
  initialize_process_system(); 

  // Enter the user shell
  shell();

  // All exits from this program will call an atexit
  return EXIT_SUCCESS;
}
