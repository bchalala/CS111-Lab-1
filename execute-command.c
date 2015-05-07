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

/*                           *
 *  Functions for execution  *
 *                           */
void execute_simple (command_t c);
void execute_pipe (command_t c);
void execute_sequence (command_t c);
void execute_and (command_t c);
void execute_or (command_t c);
void execute_subshell (command_t c);


/* time travel functions functions and stucts */
typedef struct graph_node* graph_node_t;
#define LIST_SIZE 16 // RL & WL size
void make_graph_node(command_t c);
void build_lists(command_t c, graph_node_t gn, int* write_i, int* read_i);

struct graph_node {
    char** RL;
    char** WL;
    bool dependency;
    
    struct graph_node* next;
    command_t cmd;
    pid_t pid;
    struct graph_node** depends_on;
};

// Struct that contains the graph_node_t
typedef struct { 
    graph_node_t no_dependency;
    graph_node_t dependency;
} dependency_graph;

// Create dependency_graph function definition
dependency_graph
create_dependency_graph();

graph_node_t gnode_list;
graph_node_t tail_gnode;

/* */
int
command_status (command_t c)
{
  return c->status;
}


void
main_execute( command_t c, bool time_travel) 
{
    
    if (!time_travel)
        execute_command(c);
    
    // Creates graph nodes.  
    else
    {
        if (c->type == SEQUENCE_COMMAND)
        {
            main_execute(c->u.command[0]);
            main_execute(c->u.command[1]);
        }
        else
            make_graph_node(c);
    }

}

void
timetravel_execute_command();
{
    // This is where the graph nodes are compared and executed.

    
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

/*                            *
 *   TIME TRAVEL FUNCTIONS    *
 *                            */

void make_graph_node(command_t c)
{
    graph_node_t gnode = (graph_node_t)checked_malloc(sizeof(struct graph_node));
    int write_i, read_i; // indexes for RL & WL
    write_i = read_i = 0;
    gnode->RL = (char**) checked_malloc(sizeof(char*) * LIST_SIZE);
    gnode->WL = (char**) checked_malloc(sizeof(char*) * LIST_SIZE);
    gnode->next = NULL;
    gnode->depends_on = NULL;
    gnode->cmd = c;
    gnode->dependency = false;
    
    if (gnode_list == NULL)
    {
        gnode_list = gnode;
        tail_gnode = gnode;
    }
    else
    {
        tail_gnode->next = gnode;
        tail_gnode = gnode;
    }
    
    build_lists(c, gnode, &write_i, &read_i);
}

void build_lists(command_t c, graph_node_t gn, int* write_i, int* read_i)
{
    // DFS all simple commands to get their RD's & args for input
    if (c->type == AND_COMMAND || c->type == OR_COMMAND || c->type == PIPE_COMMAND)
    {
        build_lists(c->u.command[0], gn, write_i, read_i);
        build_lists(c->u.command[1], gn, write_i, read_i);
    }
    else if (c->type == SIMPLE_COMMAND)
    {
        if (c->input != NULL)
        {
            unsigned int length = strlen(c->input) + 1;
            gn->RL[*read_i] = (char*) checked_malloc(sizeof(char) * length);
            strcpy (gn->RL[*read_i], c->input);
            (*read_i)++;
        }
        if (c->output != NULL)
        {
            unsigned int length = strlen(c->output) + 1;
            gn->WL[*write_i] = (char*) checked_malloc(sizeof(char) * length);
            strcpy (gn->WL[*write_i], c->output);
            (*write_i)++;
        }
        
        int i = 1;
        while (c->u.word[i] != NULL)
        {
            unsigned int length = strlen(c->u.word[i]) + 1;
            gn->RL[*read_i] = (char*) checked_malloc(sizeof(char) * length);
            strcpy (gn->RL[*read_i], c->u.word[i]);
            (*read_i)++; i++;
        }
    }
}

bool
does_intersect(char** list1, int l1_size, char** list2, int l2_size) 
{
   int i, j;

   // Iterates through all elements of the list, checks for intersections 
   for (i = 0; i < l1size; i++) {
      for (j = 0; j < l2size; j++) {
         if (strcmp(list1[i], list2[j]) == 0)
            return true; 
      }
   } 

   return false;
}

dependency_graph
create_dependency_graph() 
{
    dependency_graph dg;
    dg.no_dependency = NULL;
    dg.dependency = NULL;

    // Checks if the gnode_list is null or only has 1 element in it;
    graph_node_t current = gnode_list;     
    if (current == NULL)
        return dg;

    graph_node_t d_end = NULL;
    graph_node_t nd_end = NULL; 
    graph_node_t iterator = NULL;

    // Iterates through all graph_nodes, assigns their dependencies, 
    // and puts them in the correct queue
    while (current != gnode_tail)
    {
        // Moves to the next element
        iterator = current->next;

        // The cases where there is a dependency
        bool RAW = false;
        bool WAR = false;
        bool WAW = false;

        while (iterator != NULL)
        {
           RAW = does_intersect(current->RL, current->read_i, iterator->WL, current->write_i);
           WAW = does_intersect(current->WL, current->write_i, iterator->WL, current->write-i);
           WAR = does_intersect(current->WL, current->write_i, iterator->RL, current->read-i);
           
           // If there is an intersection of RAW, WAW, WAR, adds dependency
           if (RAW || WAW || WAR)
           {
                current->dependency = true;

           }
           
        }

        
        // If the current element is not dependent, adds it to no dependency
        if (current->dependency == false) 
        {
            if (nd_end == NULL) 
            {
               dg.no_dependency = current;
               nd_end = current; 
            }
            else
            {
               nd_end->next = current;
               nd_end = nd_end->next; 
            }
        }

        // Else it adds it to the end of the dependency list
        else
        {
            if (d_end == NULL) 
               dg.dependency = current;
            else
               d_end->next = current; 
        }

        // Checks for dependencies in the next element.
        current = current->next;

    }
    
    
    // Adds the last element into the dependency graph
    if (current == gnode_tail) 
    {
        
        // If the current element is not dependent, adds it to no dependency
        if (current->dependency == false) 
        {
            if (nd_end == NULL)
               dg.no_dependency = current;
            else
               nd_end->next = current;
        }

        // Else it adds it to the end of the dependency list
        else
        {
            if (d_end == NULL) 
               dg.dependency = current;
            else
               d_end->next = current; 
        }
        return dg;
    }
}






