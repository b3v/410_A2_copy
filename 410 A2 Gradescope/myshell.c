// writing our own shell
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// use isatty to see if the input is coming from
// terminal or not

#define MAX_ARGS 128
#define MAX_LENGTH 1000

volatile sig_atomic_t child_running = 0;
volatile sig_atomic_t sigint_received = 0;
volatile int background = 0;

pid_t child_pid;

// when the user presses ctrl + C
void sigint_handler(int sig) {
  sigint_received = 1;
}

int execute_command(char *command) { 
  //takes in a command and parses it to find
  //the main command and other additional flags/arguments
  //e.g. ls -l 
  //printf("strlen: %d\n", strlen(command));
  pid_t pid;
  int status;
  char *command_with_args[MAX_ARGS];

  // parent has access knowing that a child process
  // is running since we know the fork() down below
  // is about to be read.
  child_running = 1;

    int i = 0;
    
    // >, 1>, 2>
    char *output_file = NULL;
    // <
    char *input_file = NULL;
    // no default fd
    int where_to = -1;

    pid = fork();

    if (pid < 0){
      perror("fork error");
      exit(EXIT_FAILURE);
    }
    else if (pid == 0){
    // get the arguments of the command
    // the first string is the command itself
    command_with_args[i] = strtok(command, " \n");
    // while there are still flags/arguments to parse through

    /* Split Commands into Tokens; Handle I/O Redirection/Pipe Characters */
    while(command_with_args[i] != NULL && i < MAX_ARGS - 1){
      //printf("command with args: %s\n", command_with_args[i]);
      if (strcmp(command_with_args[i], ">") == 0 || strcmp(command_with_args[i], "1>") == 0){
	output_file = strtok(NULL, " ");
	// can't use this part of the command line
	// in the execvp call
	command_with_args[i] = NULL;
	i++;
	// redirect stdout
	where_to = STDOUT_FILENO;
      }
      else if (strcmp(command_with_args[i], "2>") == 0){
	output_file = strtok(NULL, " ");
	// redirec stderr
	where_to = STDERR_FILENO;
	command_with_args[i] = NULL;
	i++;
      }
      else if (strcmp(command_with_args[i], "<") == 0) {
	// get input file
	input_file = strtok(NULL, " ");
	command_with_args[i] = NULL;
	i++;
      }
      else if (strcmp(command_with_args[i], "&>") == 0) {
	// redirect stdout/stderr
	output_file = strtok(NULL, " ");
	// redirect both stdout/stderr to the output file
	where_to = STDOUT_FILENO;
	// rewire stdout to stderr 
	dup2(STDOUT_FILENO, STDERR_FILENO);
	command_with_args[i] = NULL;
	i++;
      }
      else if (strcmp(command_with_args[i], "&") == 0){
	background = 1;
	command_with_args[i] = '\0';
      }
      else if (strcmp(command_with_args[i], "|") == 0) {
	/* Handle Pipes */
	/* Code similar to pages 534-538 of Stevens and Rago */
	/* Pipe from the Child to Parent */
	int pipefd[2];
	
	if (pipe(pipefd) == -1) {
	  perror("pipe");
	  exit(EXIT_FAILURE);
	}

	int pipe_status;

	// split into parent and child process
	// for the prev_command and next_command
	pid_t pipe_pid = fork();

	if (pipe_pid < 0) {
	  perror("fork");
	  exit(EXIT_FAILURE);  
	}
	else if (pipe_pid == 0) {
	  /* Child Process */
	  // close the read end
	  close(pipefd[0]);
	  // Redirect stdout to the write end of the pipe
	  // of the parent
	  if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
	    perror("dup2");
	    exit(EXIT_FAILURE);
	  }
	  // execute the command before the pipe symbol
	  char *prev_command = NULL;

	  // problem: have to backtrack to find the command

	  // start from first token before |, e.g.
	  // -a in ls -l -a | grep rwx
	  int j = i - 1;
	  // while we don't go to before the beginning of the command line
	  while (j >= 0) {
	    // we already passed by the command earlier while
	    // we were first parsing the command line
	    if (strcmp(command_with_args[j], "|") == 0) {
	      // if a | is encountered, break the loop
	      break;
	    } else {
	      // If it's not a pipe symbol, it might be the previous command
	      // Check if it's a command by ensuring it's not an argument
	      if (command_with_args[j][0] != '-') {
                // If it's not an argument, it's the previous command
                prev_command = command_with_args[j];
                break;
	      }
	    }
	    j--;
	  }
	  
	  // Execute the previous command with its arguments using execvp
	  // Build the argument list for execvp
	  char *prev_command_args[MAX_ARGS];
	  prev_command_args[0] = prev_command;
	  prev_command_args[1] = NULL;
	  // if we have a command with arguments in the first place,
	  // e.g. ls -l -a instead of just ls
	  if (j < i - 1) {
            // If there are arguments for the previous command, add them to prev_command_args
            int k = 1;
            while (j < i - 1) {
	      // the current position of the arg in the
	      // consecutive arg we encountered in command_with_args
	      prev_command_args[k] = command_with_args[j + 1];
	      j++;
	      k++;
            }
	    // null terminate like we did with strtok earlier
            prev_command_args[k] = NULL;
	  }
	  
	  if (execvp(prev_command, prev_command_args) == -1) {
	    perror("execvp");
	    exit(EXIT_FAILURE);
	  }
	}
	else {
	  /* Parent Process on Pipe */
	  // close the read end of the pipe
	  close(pipefd[1]);
	  
	  // redirect stdin to the read end of the pipe
	  // so the command goes into pipe
	  if (dup2(pipefd[0], STDIN_FILENO) == -1) {
	    perror("dup2");
	    exit(EXIT_FAILURE);
	  }	  
	  // execute the command after the pipe symbol
	  char *next_command = strtok(NULL, "|");

	  int n = 0;
	  char *next_command_args[MAX_ARGS];

	  char io[MAX_ARGS][MAX_ARGS] = {">", "<", "1>", "2>", "&>"};
	  
	  next_command_args[n] = strtok(next_command, " ");
	  int get_out = 0;
	  while (next_command_args[n] != NULL){
	    for (int m = 0; m < MAX_ARGS; m++) {
	      if(strcmp(next_command_args[n], io[m]) == 0){
		get_out = 1;
		  break;
	      }
	    }
	    if(get_out == 1)
		break;
	    n++;
	    next_command_args[n] = strtok(NULL, " ");
	  }
	  next_command_args[n] = NULL;
	  
	  // wait for prev command to finish
	  waitpid(pipe_pid, &pipe_status, 0); 
	  
	  if (execvp(next_command_args[0], next_command_args) == -1) {
	    perror("execvp");
	    exit(EXIT_FAILURE);
	  }
	}
      }
      i++;
      command_with_args[i] = strtok(NULL, " \n");
    }
    command_with_args[i] = NULL; // Null-terminate; required when using strtok
    
    /* Handling I/O Redirection */

    // if input redirection is requested
    if (input_file != NULL) {
      int fd = open(input_file, O_RDONLY);
      if (fd == -1) {
	perror("open");
	exit(EXIT_FAILURE);
            }
      // Redirect input file to stdin
      if (dup2(fd, STDIN_FILENO) == -1) {
	perror("dup2");
	exit(EXIT_FAILURE);
      }
      close(fd);
    }
    
    // if output redirection is requested
    if (output_file != NULL) {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
	perror("open");
	exit(EXIT_FAILURE);
	}
      // essentially rewire the file's fd to connect to STDOUT
      // so whatever we write to stdout goes into the file
      if (dup2(fd, where_to) == -1) {
	perror("dup2");
	exit(EXIT_FAILURE);
      }
      close(fd);
    }
      /* Executing Command */
      if (execvp(command_with_args[0], command_with_args) == -1){
	perror("execvp issue\n");
	return -1;
      }
    }
    else { // work in parent process address space
    // Wait for the child process to finish
    // i.e. go to the block where pid == 0

    if (!background){
      // if no &, wait for the process to finish
      waitpid(pid, &status, 0);
    }
    //else if (background) {
      // If a background process, don't wait
      //background = 0;
      //printf("[%d] %s\n", pid, command);
      //}
    child_running = 0;
  }
  return 0;
}

int parse_command_line(){

  char command_line[MAX_LENGTH];
  char *args[MAX_ARGS];
  char *output_file = NULL;
  int in_out;
  
  // read the command line with a max buffer size of 1000,
  // where the command line is stdin
  fgets(command_line, 1000, stdin) == NULL;
  
  /* Parse the command line by each ; */
  int i = 0;
  // get the first command
  args[i] = strtok(command_line, ";\n");

  // get the commands
  while (args != NULL && i < MAX_ARGS - 1) {
    // start from 1
    i++;
    args[i] = strtok(NULL, ";\n");
  }
  args[i] = NULL; // Null-terminate the command list
  
  // once all of the commands are collected
  int j = 0;
  while(args[j] != NULL){
    // create a child process for each of the commands in the argument list:
    if (execute_command(args[j]) == -1){
      perror("failed to make child process");
      return -1;
    }
    // go to the next command and its arguments
    j++;
  }
  return 0;
}


int main(){
  /* Main Function: */
  /* Handle SIGINT; Handle tty input; Running the Shell */
  
  // install signal handler for Ctrl+C
  if (signal(SIGINT, sigint_handler) == SIG_ERR) {
    perror("Could not install signal handler");
    return 1;
  }
  while(1){
    if(isatty(STDIN_FILENO)){
      printf("myshell> ");
      fflush(stdout);
    }
    if (sigint_received) {
      sigint_received = 0;
      // If a child process is running, send a SIGINT to it
      if (child_running) {
	printf("Killing child process\n");
	kill(child_pid, SIGINT);
      }
    }
    
    if (parse_command_line() == -1) {
      printf("Error executing command\n");
    }
  }
  return 0;
}
