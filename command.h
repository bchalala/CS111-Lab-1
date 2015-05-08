// UCLA CS 111 Lab 1 command interface
// Brett Chalabian
// Chul Hee Woo


#include <stdbool.h>

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;
typedef struct command_stack command_stack;
typedef struct token token;


/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the flag is set.  */
void execute_command (command_t);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);

/////////////////////////////////////////////////////////////////////
/* Execute Functions */
/////////////////////////////////////////////////////////////////////

void execute_simple (command_t c);

void execute_sequence (command_t c);

void execute_subshell (command_t c);

void execute_and (command_t c);

void execute_or (command_t c);

void execute_pipe (command_t c);

/////////////////////////////////////////////////////////////////////
/* Functions defined by me for help with making the command stream */
/////////////////////////////////////////////////////////////////////

// Creates tokens from the char buffer
void tokenize (char* buf, int file_length, char** words, unsigned int* wordCount, token** token_stream);

/* Creates a single command tree from a starting element of the word_arr. 
   The final element is set to the final \n character, or the EOF. */
command_t create_tree (token *token_arr, unsigned int *arr_size, int *element);

// Creates individual commands
bool create_command(token token, bool *flags, command_t command, command_t prev_command);

// Handles flags for the create_tree and throws errors
bool handle_flags(token token, bool *flags);

// Sets the flags for the create_tree
void set_flags(token token, bool *flags);

// Balances the stacks for the algorithm infix to postfix
void balance_stacks(command_stack *cmdstack, command_stack *opstack, command_t new_command);

// General exit function
void exit_message(const char* message);



/////////////////////////////////////////////////////////////////////
/* Command Stack Function definitions */
/////////////////////////////////////////////////////////////////////

// Pushes a command pointer to the stack
void push(command_stack *stack, command_t command);

// Pops a command pointer from the stack
command_t pop(command_stack *stack);

// Initializes the stack to the base value
void init(command_stack *stack);


/////////////////////////////////////////////////////////////////////
/* Test Functions */
/////////////////////////////////////////////////////////////////////

// Testing function for the stack
void testStack();

// Test function for tree generation
void testTree();
