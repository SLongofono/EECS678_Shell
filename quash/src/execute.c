/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>

#include "quash.h"

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
	fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {

	long max_path = fpathconf(0, _PC_PATH_MAX);

	// If no limit set, use a good rule of thumb value
	if(max_path < 0){
		max_path = 4096;
	}

	// Change this to true if necessary
	*should_free = true;

	char *temp = malloc(4096);
	getcwd(temp, max_path);

	return temp;//getcwd((char*)NULL, max_path);  //"get_current_directory()";

}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {

	return getenv(env_var);

}

// Check the status of background jobs
void check_jobs_bg_status() {

	// TODO: Check on the statuses of all processes belonging to all background
	// jobs. This function should remove jobs from the jobs queue once all
	// processes belonging to a job have completed.
	IMPLEMENT_ME();

	// TODO: Once jobs are implemented, uncomment and fill the following line
	// print_job_bg_complete(job_id, pid, cmd);
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
	printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
	fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
	printf("Background job started: ");
	print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
	printf("Completed: \t");
	print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Helpers
 **************************************************************************/

//void handle_bg_process(pid_queue bg_q, int operation



/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {

	// Execute a program with a list of arguments. The `args` array is a NULL
	// terminated (last string is always NULL) list of strings. The first element
	// in the array is the executable
	char* exec = cmd.args[0];
	char** args = cmd.args;

	execvp(exec, args);

	perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
	// Print an array of strings. The args array is a NULL terminated (last
	// string is always NULL) list of strings.
	char** str = cmd.args;
	while(NULL != *str){
		printf("%s", *str);
		str++;
	}

	putchar('\n');

	// Flush the buffer before returning
	fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {

	// Simply try to set the environment varible to the given value, no
	// need to check that it is accurate
	setenv(cmd.env_var, cmd.val, 1);
	
}

// Changes the current working directory
void run_cd(CDCommand cmd) {

	// Get the directory name
	const char* dir = cmd.dir;

	// Check if the directory is valid
	if (dir == NULL) {
		perror("ERROR: Failed to resolve path");
		return;
	}

	const char *temp = getenv("PWD");

	// Actually change the working directory
	if(0 != chdir(dir) ){
		perror("ERROR: Failed to change directory");
		return;
	}

	// Change environment variables
	if( 0 != setenv("OLDPWD", temp, 1) ){
		perror("ERROR: Failed to update OLDPWD");
		return;
	}
	else if( 0 != setenv("PWD", dir, 1) ){
		perror("ERROR: Failed to update PWD");
		return;
	}
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {

	int signal = cmd.sig;
	int job_id = cmd.job;

	// TODO: Remove warning silencers
	(void) signal; // Silence unused variable warning
	(void) job_id; // Silence unused variable warning

	// TODO: Kill all processes associated with a background job
	IMPLEMENT_ME();
}


// Prints the current working directory to stdout
void run_pwd() {

	// Per instructions, we can't just print the value of $PWD
	long max_path = fpathconf(0, _PC_PATH_MAX);
	
	if(0 > max_path){
		max_path =4096;	
	}

	char *temp = malloc(max_path);
	//char *temp;

	// Uses Malloc
	//temp = get_current_dir_name();
	
	getcwd(temp, max_path);

	printf("%s\n", temp);
	
	// Flush the buffer before returning
	fflush(stdout);
	
	free(temp);

}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {

	// Need a temp queue to actually get at all the elements
	pid_queue temp_q = new_pid_queue(10);

	// Unload everything into temp q
	while(! is_empty_pid_queue(&bg_q)){
		push_back_pid_queue(&temp_q, (pop_front_pid_queue(&bg_q)) );	
	}

	// Reload bg_q, printing as you go
	while(! is_empty_pid_queue(&temp_q)){
		printf("%d\n", peek_front_pid_queue(&temp_q));
		push_back_pid_queue(&bg_q, (pop_front_pid_queue(&temp_q)));
	}

	destroy_pid_queue(&temp_q);

	// Flush the buffer before returning
	fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {

	CommandType type = get_command_type(cmd);

	switch (type) {
	case GENERIC:
		run_generic(cmd.generic);
		break;

	case ECHO:
		run_echo(cmd.echo);
		break;

	case PWD:
		run_pwd();
		break;

	case JOBS:
		run_jobs();
		break;

	case EXPORT:
	case CD:
	case KILL:
	case EXIT:
	case EOC:
		break;

	default:
		fprintf(stderr, "Unknown command type: %d\n", type);
	}
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {

	CommandType type = get_command_type(cmd);

	switch (type) {
	case EXPORT:
		run_export(cmd.export);
		break;

	case CD:
		run_cd(cmd.cd);
		break;

	case KILL:
		run_kill(cmd.kill);
		break;

	case GENERIC:
	case ECHO:
	case PWD:
	case JOBS:
	case EXIT:
	case EOC:
		break;

	default:
		fprintf(stderr, "Unknown command type: %d\n", type);
	}
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder) {

	// Read the flags field from the parser
	bool p_in  = holder.flags & PIPE_IN;
	bool p_out = holder.flags & PIPE_OUT;
	bool r_in  = holder.flags & REDIRECT_IN;
	bool r_out = holder.flags & REDIRECT_OUT;
	bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
						     // is true
						     //
	int fd[2];
	pipe(fd);	
	
	

	int pid = fork();
	if(0 == pid){  // Child process
		if(p_in || p_out){
			// Case input pipe only
			if(p_in && !p_out){
				dup2(fd[0], 0);
				close(fd[1]);
			}
			else if(p_out && !p_in){  // Case output pipe only
				dup2(fd[1], 1);
				close(fd[0]);

			}
			else{ // Case both input and output pip
				dup2(fd[0], 0);
				dup2(fd[1], 1);
			}
		}
		else{   // Case no use for pipes
			close(fd[0]);
			close(fd[1]);

			if(r_in){
				// Open the file at the given path in read only mode
				int fp = open(holder.redirect_in, O_RDONLY);
				if (fp < 0){
					perror("Error: could not open file for input redirection");
					return;
				}
	
				// Duplicate file descriptor to replace stdin
				dup2(fp, 0);
	
				// close now that we don't need it anymore
				close(fp);
			}
			if(r_out){
				if(r_app){
	
					// open the file at the given path in
					// read/write/create/append mode
					int fp = open(holder.redirect_out, O_RDWR | O_APPEND | O_CREAT, 0666);
	
					if(fp < 0){
						perror("ERROR: could not open file for output redirection");
						return;
					}
	
					// duplicate the file descriptor to replace
					// stdout
					dup2(fp, 1);
	
					// close now that we don't need it anymore
					close(fp);
				}
				else{
					
					// open the fail at the given path in
					// read/write/create mode
					int fp = open(holder.redirect_out, O_RDWR | O_CREAT, 0666 );
	
					if(fp < 0){
						perror("Error: could not open file for output redirection");
						return;	
					}
	
					// duplicate the file descriptor to replace
					// stdout
					dup2(fp, 1);
	
					// close now that we don't need it anymore
					close(fp);
	
				}
			}
		}
		child_run_command(holder.cmd); // This should be done in the child branch of a fork
		exit(0);
	}// end child process block
	else{
		parent_run_command(holder.cmd); // This should be done in the parent branch of
			                        // a fork
	}
	// Add the child to the active foreground process queue
	push_back_pid_queue(&process_q, pid);
	close(fd[0]);
	close(fd[1]);
}

// Run a list of commands
void run_script(CommandHolder* holders) {

	if (holders == NULL)
		return;

	check_jobs_bg_status();

	if (get_command_holder_type(holders[0]) == EXIT &&
	    get_command_holder_type(holders[1]) == EOC) {
		end_main_loop();
		return;
	}

	CommandType type;

	// Run all commands in the `holder` array
	// This generates all the processes and connects pipes/redirs
	// appropriately, and adds pids to the foreground process queue
	for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
		create_process(holders[i]);

	if (!(holders[0].flags & BACKGROUND)) {

		// We need to wait for each to complete, then remove it from
		// our queue of foreground processes
		while(!is_empty_pid_queue(&process_q)){

			int active, status;
			// Grab the first item (presumably the oldest if we
			// always add to the back)
			active = pop_front_pid_queue(&process_q);

			// Block until the associated process exits
			waitpid(active, &status, 0);

			// Do we want to error check here, or is that the
			// user's responsibility to give good input?			
		}
	}
	else {
		// A background job.
		// TODO: Push the new job to the job queue
		
		//IMPLEMENT_ME();
		//int pid = pop_front_pid_queue(&process_q);
		//push_front_pid_queue(&bg_q, pid);


		// TODO: Once jobs are implemented, uncomment and fill the following line
		//print_job_bg_start(job_id, pid, holders[0].cmd);
	}
}
