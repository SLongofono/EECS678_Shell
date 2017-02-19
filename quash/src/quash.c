/**
 * @file quash.c
 *
 * Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/
#include "quash.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "command.h"
#include "execute.h"
#include "parsing_interface.h"
#include "memory_pool.h"

/**************************************************************************
 * Private Variables
 **************************************************************************/
static QuashState state;

/**************************************************************************
 * Private Functions
 **************************************************************************/
static QuashState initial_state() {
	return (QuashState) {
		true,
		isatty(STDIN_FILENO),
		NULL
	};
}

// Print a prompt for a command
static void print_prompt() {
	bool should_free = true;
	char* cwd = get_current_directory(&should_free);

	assert(cwd != NULL);

	char hostname[HOST_NAME_MAX];

	// Get the hostname
	gethostname(hostname, HOST_NAME_MAX);

	// Remove first period and everything afterwards
	for (int i = 0; hostname[i] != '\0'; ++i) {
		if (hostname[i] == '.') {
			hostname[i] = '\0';
			break;
		}
	}

	char* last_dir = cwd;
	// Show only last directory
	for (int i = 0; cwd[i] != '\0'; ++i) {
		if (cwd[i] == '/' && cwd[i + 1] != '\0') {
			last_dir = cwd + i + 1;
		}
	}

	char* username = getlogin();

	// print the prompt
	printf("[QUASH - %s@%s %s]$ ", username, hostname, last_dir);

	fflush(stdout);

	if (should_free)
		free(cwd);
}

/**************************************************************************
 * Public Functions
 **************************************************************************/
// Check if loop is running
bool is_running() {
	return state.running;
}

// Get a copy of the string
char* get_command_string() {
	return strdup(state.parsed_str);
}

// Check if Quash is receiving input from the command line or not
bool is_tty() {
	return state.is_a_tty;
}

// Stop Quash from requesting more input
void end_main_loop() {
	state.running = false;
}

/**
 * @brief Quash entry point
 *
 * @param argc argument count from the command line
 *
 * @param argv argument vector from the command line
 *
 * @return program exit status
 */
int main(int argc, char** argv) {
	state = initial_state();
	
	
	/*    EXAMPLE OF USAGE, job struct, job struct q (bg_q), and pid-queue
	 *    (process_q)
	test.job_id = 1985;
	bg_q = new_job_queue(10);
	process_q = new_pid_queue(10);
	test.process_q = &process_q;
	
	pid_queue temp = new_pid_queue(1);
	push_front_pid_queue(&temp, 1);
	push_front_pid_queue(&temp, 2);
	push_front_pid_queue(&temp, 3);
	test.process_q = &temp;
	push_front_job_queue(&bg_q, test);
	int firstJob = peek_front_job_queue(&bg_q).job_id;
	int firstPid = peek_front_pid_queue(peek_front_job_queue(&bg_q).process_q);
	printf("The first job in the bg_q has id %d, and the first process in that job has id %d\n", firstJob, firstPid);

	printf("Struct test:\n\tSize: %ld \n\tid: %d\n", sizeof(test), test.job_id);
	*/


	if (is_tty()) {
		puts("Welcome to Quash!");
		puts("Type \"exit\" or \"quit\" to quit");
		puts("---------------------------------");
		fflush(stdout);
	}

	atexit(destroy_parser);
	atexit(destroy_memory_pool);

	// Main execution loop
	while (is_running()) {
		if (is_tty())
			print_prompt();

		initialize_memory_pool(1024);
		CommandHolder* script = parse(&state);

		if (script != NULL)
			run_script(script);

		destroy_memory_pool();
	}

	return EXIT_SUCCESS;
}
