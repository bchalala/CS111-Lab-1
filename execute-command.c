// UCLA CS 111 Lab 1 command execution
// Brett Chalabian
// Chul Hee Woo


#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <stdlib.h>
#include <error.h>


// added libraries
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


typedef struct {
    
} GraphNode; 

typedef struct {
    GraphNode* no_dependencies;
    GraphNode* dependencies;
} DependencyGraph;



void execute_simple (command_t c);
void execute_pipe (command_t c);
void execute_sequence (command_t c);
void execute_and (command_t c);
void execute_or (command_t c);
void execute_subshell (command_t c);

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c)
{
    switch (c->type) {
            
        case SIMPLE_COMMAND:
            execute_simple(c);
            break;
            
        case PIPE_COMMAND:
            execute_pipe(c);
            break;
            
        case SEQUENCE_COMMAND:
            execute_sequence(c);
            break;
            
        case AND_COMMAND:
            execute_and(c);
            break;
            
        case OR_COMMAND:
            execute_or(c);
            break;
            
        case SUBSHELL_COMMAND:
            execute_subshell(c);
            break;
            
        default:
            exit_message("Problem with command, not one of the proper types. \n");
    }
}

static int
rd_execute (char *io, bool *b, int des, int *backup)
{
    if (io)
    {
        // Sets the redirection to true, backs up
        *b = true;
        *backup = dup(des);
        
        // Aliases the file descriptor and returns it.
        int fd = open(io, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (fd < 0)
            exit_message("File Descriptor failed to open. Exiting. \n");
        dup2(fd, des);
        return fd;
    }
    
    return -1;
}


void
execute_simple (command_t c) {
    
    // This function can only run simple commands. If it is not simple, then creates an error.
    if (c->type != SIMPLE_COMMAND)
        exit_message("Error: execute_simple called without a simple command. \n");
    
    // If there is a redirect, these take care of it.
    bool in = false;
    bool out = false;
    int fdin = -1;
    int fdout = -1;
    
    // Backups of stdin & stdout
    int b_in = -1;
    int b_out = -1;
    
    // Redirects the i/o if possible
    fdin = rd_execute(c->input, &in, 0, &b_in);
    fdout = rd_execute(c->output, &out, 1, &b_out);
    
    int pid = fork();
    
    // Child case: it should execute the command. If it doesn't, it exits with 127.
    if (pid == 0)
    {
        execvp(c->u.word[0], c->u.word);
        
        // If execvp fails, then it exits with 127. 
        exit(127);
    }
    
    // Parent: Get the exit status of the command and store it in c->status.
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        int exit_status = WEXITSTATUS(status);
        c->status = exit_status;
    }
    else
        exit_message("FORK UNSUCCESSFUL.");
    
    // If input has been redirected, then restore stdin.
    if (in) {
        dup2(b_in, 0);
        close(fdin);
    }
    
    // If output has been redirected, then store stdout.
    if (out) {
        dup2(b_out, 1);
        close (fdout);
    }
}

// Function to execute or commands.
void execute_or (command_t c){
    
    // Executes the first command
    execute_command(c->u.command[0], time_travel);
    int status = c->u.command[0]->status;
    
    // If the first command does not return true, executes the second.
    if (status != 0) {
        execute_command(c->u.command[1], time_travel);
        status = c->u.command[1]->status;
    }
    
    // Sets the status of the OR equal to 0 if either command executes correctly.
    c->status = status;
    
    return;
}

void
execute_subshell (command_t c) {
    if (c->type != SUBSHELL_COMMAND)
        exit_message("Error: execute_subshell called without subshell command. \n");
    
    // If there is a redirect, these take care of it.
    bool in = false;
    bool out = false;
    int fdin = -1;
    int fdout = -1;
    
    // Backups of stdin & stdout
    int b_in = -1;
    int b_out = -1;
    
    // Redirects the i/o if possible
    fdin = rd_execute(c->input, &in, 0, &b_in);
    fdout = rd_execute(c->output, &out, 1, &b_out);
    
    // Calls execute command on the subshell_commmand
    execute_command(c->u.subshell_command, time_travel);
    
    // If input has been redirected, then restore stdin.
    if (in) {
        dup2(b_in, 0);
        close(fdin);
    }
    
    // If output has been redirected, then store stdout.
    if (out) {
        dup2(b_out, 1);
        close (fdout);
    }
    
}



// Function to execute and commands.
void execute_and (command_t c)
{
	execute_command(c->u.command[0], time_travel);

	if (c->u.command[0]->status == 0) {	// only if first command succeeds
		execute_command(c->u.command[1], time_travel);
		c->status = c->u.command[1]->status;
	}
	else
		c->status = c->u.command[0]->status;
	return;
}



// Function to execute pipe commands.
void execute_pipe (command_t c) {

	int pipefd[2];
	if (pipe(pipefd) == -1)
		error(1, 0, "PIPE ERROR");

	pid_t pid = fork();

	if (pid == 0)	// child will execute the first command
	{	
		close(pipefd[0]);	// no need for read end	
		dup2(pipefd[1], 1);
		execute_command(c->u.command[0], time_travel);
		close(pipefd[1]);
		exit(c->u.command[0]->status);
	}
	else if (pid > 0)	// vice versa with parent
	{	
		pid_t pid2 = fork();
		if (pid2 == 0)
		{
			close(pipefd[1]);
			dup2(pipefd[0],0);
			execute_command(c->u.command[1], time_travel);
			close(pipefd[0]);
			exit(c->u.command[1]->status);

		}
		else if  (pid2 > 0)
		{
			close(pipefd[0]);
			close(pipefd[1]);
			int status;
       		 	pid_t waiting = waitpid(-1, &status, 0);

			if (waiting == pid2)
			{
				waitpid(pid, &status, 0);
				c->status = WEXITSTATUS(status);
				return;
			}
        		else if (waiting == pid)
			{
				c->status = WEXITSTATUS(status);
				waitpid(pid2, &status, 0);
				return;
			}
		}
		else
			error(1, 0, "FORK ERROR IN PIPE");
	}
	else
		error(1, 0, "FORK ERROR IN PIPE");	
    return;
}

void execute_sequence (command_t c) 
{
	int status;
	execute_command(c->u.command[0], time_travel);
	execute_command(c->u.command[1], time_travel);
	status = c->u.command[1]->status;
	c->status = status;
	return;
}
