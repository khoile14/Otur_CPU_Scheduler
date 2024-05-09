/* Standard Library Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* Local Includes */
#include "otur_sched.h" // Your schedule for the functions you're testing.
#include "vm_support.h" // Gives ABORT_ERROR, PRINT_WARNING, PRINT_STATUS, PRINT_DEBUG commands

/* Globals (static means it's private to this file only) */
int g_debug_mode = 1; // Hardcodes debug on for the custom print functions
 
/* Local Prototypes */
Otur_schedule_s *createSchedule();
void test_otur_initialize();
void test_otur_invoke();
void test_otur_enqueue();
void test_otur_select();
void test_otur_promote();
void test_otur_exited();
void test_otur_reap();
static void test_queue_initialized(Otur_queue_s *queue);

/* This is an EXAMPLE tester file, change anything you like!
 * - This shows an example by testing otur_initialize.
 * - To make this useful, modify this function or create others for the rest of the
 *   functions that you want to test from your code.
 */
int main() {
  g_debug_mode = 1;  // These helper functions only print if g_debug_mode is 1

  // Here I'm Calling a local helper function to test your otur_initialize code.
  // - You can make any helper functions you like to test it however you want!
  
  // PRINT_STATUS is a helper macro to print a message when you run the code.
  // - The definitions are in int/_support.h
  // - It works just like printf, but it prints out in some nice colors.
  // - It also adds a newline at the end, so you don't need to here.
  // - The PRINT_* macros are available for your otur_sched.c as well, if you want to use them.
  PRINT_STATUS("Test 1: Testing otur_initialize");
  test_otur_initialize();
  PRINT_STATUS("Test 2: Testing otur_invoke");
  test_otur_invoke();
  PRINT_STATUS("Test 3: Testing otur_enqueue");
  test_otur_enqueue();
  PRINT_STATUS("Test 4: Testing otur_select");
  test_otur_select();
  PRINT_STATUS("Test 5: Testing otur_promote");
  test_otur_promote();
  PRINT_STATUS("Test 6: Testing otur_exited");
  test_otur_exited();
  PRINT_STATUS("Test 7: Testing otur_reap");
  test_otur_reap();

  // You would add more calls to testing helper functions that you like.
  // Then when done, you can print a nice message an then return.
  PRINT_STATUS("All tests complete!");
  return 0;
}

/* Local function to test schedule_initialize from otur_sched.c */
void test_otur_initialize() {
  /* PRINT_DEBUG, PRINT_STATUS, PRINT_WARNING, PRINT_ERROR, and ABORT_ERROR are all 
   *   helper macros that we wrote to print messages to the screen during debugging.
   * - Feel free to use these, or not, based on your own preferences.
   * - These work just like printf, but print different labels and they add in a newline at the end.
   * BTW: gdb works muuuuuch better in this test file than it does in 
   */
  PRINT_DEBUG("...Calling otur_initialize()");

  /* Note: MARK is a really cool helper macro.  Like the others, it works just like printf.
   * It also prints out the file, function, and line number that it was called from.
   * You can use it to check values throuhgout the code and mark where you printed from.
   * BTW: gdb also does this much better too.
   */
  MARK("I can be used anywhere, even if debug mode is off.\n");
  MARK("I work just like printf! %s %d %lf\n", "Cool!", 42, 3.14);

  // Begin Testing
  // - You can just call the function you want to test with any arguments needed.
  // - You may need to set up some arguments first, depending on what you want to test.
  Otur_schedule_s *schedule = otur_initialize();
  
  // Now that we called it and got the pointer to the schedule, let's test to see if we did it right!
  if(schedule == NULL) {
    // ABORT_ERROR will kill the program when it hits, which is good if you hit a bug.
    ABORT_ERROR("...otur_initialize returned NULL!"); 
  }
  // Header is good, so let's test the queues to see if they're all initialized properly.
  PRINT_STATUS("...Checking the Ready Queue - High");
  test_queue_initialized(schedule->ready_queue_high); // This is another helper function I wrote in this file.
  PRINT_STATUS("...Checking the Ready Queue - Normal");
  test_queue_initialized(schedule->ready_queue_normal); // This is another helper function I wrote in this file.
  PRINT_STATUS("...Checking the Defunct Queue");
  test_queue_initialized(schedule->defunct_queue); // This is another helper function I wrote in this file.

  // Last example code, how to print the schedule of all three linked lists.
  PRINT_STATUS("...Printing the Schedule");
  // You can use print_otur_debug to print out all of the schedules.
  // - Note: the second argument is for the Process on the CPU
  //         When testing, if you select a process, you can pass a pointer to that into the
  //           second argument to have it print out nice.
  // - In this case, we haven't selected any Process, so passing in NULL
  print_otur_debug(schedule, NULL); // the second argument here is used for the process on the CPU.
  PRINT_STATUS("...otur_initialize is looking good so far.");
}

/* Helper function to test if a queue is properly initialized
 * Exits the program with ABORT_ERROR on any failures.
 */
static void test_queue_initialized(Otur_queue_s *queue) {
  // Always test a pointer before dereferencing it!
  if(queue == NULL) {
    ABORT_ERROR("...tried to test a NULL queue!");
  }

  // Check that the head pointer is NULL.
  if(queue->head != NULL) {
    ABORT_ERROR("...the Queue doesn't have a NULL head pointer!");
  }
  // Check that the count is 0.
  if(queue->count != 0) {
    ABORT_ERROR("...the Queue's count is not initialized to 0!");
  }
}

void test_otur_invoke() {
  Otur_process_s *node1 = otur_invoke(1, 0, 0, "Node 1");
  printf("PID: %d, Age: %d, CMD: %s, State %hx\n", node1->pid, node1->age, node1->cmd, node1->state);
}

// Helper functions
Otur_schedule_s *createSchedule() {
    Otur_schedule_s *schedule = otur_initialize();
    Otur_process_s *node1 = otur_invoke(1, 1, 0, "node 1");
    Otur_process_s *node2 = otur_invoke(2, 1, 1, "node 2");
    Otur_process_s *node3 = otur_invoke(3, 0, 0, "node 3");
    Otur_process_s *node4 = otur_invoke(4, 0, 0, "node 4");

    otur_enqueue(schedule, node1);
    otur_enqueue(schedule, node2);
    otur_enqueue(schedule, node3);
    otur_enqueue(schedule, node4);

    return schedule;
}

void printHigh(Otur_schedule_s *schedule) {
    Otur_process_s *current = schedule->ready_queue_high->head;
    printf("Ready Queue (High) [%d]:\n", schedule->ready_queue_high->count);
    while (current != NULL) {
        printf("%s (State: %hx, Age: %hx)\n", current->cmd, current->state, current->age);
        current = current->next;
    }
}

void printNormal(Otur_schedule_s *schedule) {
    Otur_process_s *current = schedule->ready_queue_normal->head;
    printf("Ready Queue (Normal) [%d]:\n", schedule->ready_queue_normal->count);
    while (current != NULL) {
        printf("%s (State: %hx, Age: %hx)\n", current->cmd, current->state, current->age);
        current = current->next;
    }
}

void printDefunct(Otur_schedule_s *schedule) {
    Otur_process_s *current = schedule->defunct_queue->head;
    printf("Defunct Queue[%d]:\n", schedule->defunct_queue->count);
    while (current != NULL) {
        printf("%s (State: %hx, Age: %hx)\n", current->cmd, current->state, current->age);
        current = current->next;
    }
}

void test_otur_enqueue() {
    Otur_schedule_s *schedule = createSchedule();

    printHigh(schedule);
    printNormal(schedule);
}

void test_otur_select() {
    Otur_schedule_s *schedule = createSchedule();
    int counter = 0, num_nodes = schedule->ready_queue_normal->count + schedule->ready_queue_high->count;

    while (counter < num_nodes) {
        printf("Select %d:\n", counter+1);
        otur_select(schedule);
        printHigh(schedule);
        printNormal(schedule);
        printDefunct(schedule);
        printf("\n");
        counter++;
    }

}

void test_otur_promote() {
    Otur_schedule_s *schedule = createSchedule();
    int i;

    printf("Initial queue state:\n");
    printHigh(schedule);
    printNormal(schedule);
    for (i = 1; i <= 5; i++) {
        printf("Promote %d:\n", i);
        otur_promote(schedule);
        printHigh(schedule);
        printNormal(schedule);
    }
}

void test_otur_exited() {
    Otur_schedule_s *schedule = createSchedule();
    int count = 0, num_nodes = schedule->ready_queue_normal->count + schedule->ready_queue_high->count;

    printf("Queues in original state:\n");
    printHigh(schedule);
    printNormal(schedule);
    printDefunct(schedule);

    printf("Queues after...\n");
    while (count < num_nodes) {
        otur_exited(schedule, otur_select(schedule), 1);
        printf("Exit [%d]:\n", count+1);
        printHigh(schedule);
        printNormal(schedule);
        printDefunct(schedule);
        count++;
    }
}

void test_otur_reap() {
    Otur_schedule_s *schedule = createSchedule();
    int exit_code, count = 0, num_nodes = schedule->ready_queue_normal->count + schedule->ready_queue_high->count;

    while (count < num_nodes) {
        otur_exited(schedule, otur_select(schedule), 1);
        count++;
    }
    printf("Original Queue State:\n");
    printHigh(schedule);
    printNormal(schedule);
    printDefunct(schedule);
    printf("\n");

    exit_code = otur_reap(schedule, 0);
    printf("After otur_reap(0) (Exit code %d):\n", exit_code);
    printHigh(schedule);
    printNormal(schedule);
    printDefunct(schedule);
    printf("\n");

    exit_code = otur_reap(schedule, 3);
    printf("After otur_reap(3) (Exit code %d):\n", exit_code);
    printHigh(schedule);
    printNormal(schedule);
    printDefunct(schedule);
    printf("\n");

    exit_code = otur_reap(schedule, 4);
    printf("After otur_reap(4) (Exit code %d):\n", exit_code);
    printHigh(schedule);
    printNormal(schedule);
    printDefunct(schedule);
    printf("\n");

    exit_code = otur_reap(schedule, 1);
    printf("After otur_reap(1) (Exit code %d):\n", exit_code);
    printHigh(schedule);
    printNormal(schedule);
    printDefunct(schedule);
    printf("\n");
}
