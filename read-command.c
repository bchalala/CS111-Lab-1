// UCLA CS 111 Lab 1 command reading
// Brett Chalabian
// Chul Hee Woo

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
// #include <error.h>


// Flag definitions
#define NL_FLAG 0
#define RDIN_FLAG 1
#define RDOUT_FLAG 2
#define SIMPLE_FLAG 3
#define SIMPLE_REQ_FLAG 4
#define RDIN_HANDLED_FLAG 5
#define RDOUT_HANDLED_FLAG 6
#define RDOK_FLAG 7
#define END_FLAG 8

// Number of flags
#define NUMFLAGS 9



/////////////////////////////////////////////////////////////////////
/* Command Stream, Node, and Stack Struct Definitions */
/////////////////////////////////////////////////////////////////////

struct command_node
{
    struct command_node *next;
    struct command *command;
};

struct command_stream
{
    // Keeps track of the first and last elements for easy placement.
    struct command_node *head;
    struct command_node *tail;
    
    struct command_node *cur;
    
};

/* Implementation for command_stack */

struct command_stack
{
    command_t *stack;
    int size;
};

/* Different possible token types */

enum token_type
{
    WORD,       // Simple
    AND,        // &&
    SEQ,        // ;
    OR,         // |
    PIPE,       // ||
    LEFT,       // (
    RIGHT,      // )
    LESS,       // <
    GREATER,    // >
    NEWLINE     // \n
};

/* Struct to manage the individual tokens */

struct token
{
    enum token_type type;
    char* content;
};


/////////////////////////////////////////////////////////////////////
/* Helper Functions */
/////////////////////////////////////////////////////////////////////


bool isValid (char c)
{
    return isalnum(c) || c == '!' || c == '%' || c == '+' || c == ',' || c == '-' ||
    c == '.' || c == '/' || c == ':' || c == '@' || c == '^' || c == '_';
}

bool isCmd (char c)
{
    return c == ';' || c == '|' || c == '&' || c == '(' || c == ')' || c == '<' || c == '>';
}

bool isComment(char c)
{
    return (c == '#');
}


/////////////////////////////////////////////////////////////////////
/* Command stream read and command stream make definitons */
/////////////////////////////////////////////////////////////////////


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{
    size_t bufSize = 1024;
    size_t currSize = bufSize;
    char* buffer = (char*) checked_malloc(sizeof(char) * bufSize);
	if (buffer == NULL)
	{
        exit_message("Buffer not allocated. \n");
    }

    unsigned int length = 0;
    int a = 0;
    
    while ((a = get_next_byte(get_next_byte_argument)) != EOF)
    {
        if (!isValid(a) && !isCmd(a) && !isComment(a) && !isspace(a))
            exit_message("Invalid Character. \n");
        
        if (length == currSize) // currSize - 1?
        {
            checked_grow_alloc(buffer, &currSize);
            currSize *= 2;
        }
        
        buffer[length] = a;
        length++;
    }

	buffer[length] = '\0';

    char** words = NULL;
    token* token_array = NULL;
    token** token_stream = &token_array;
    unsigned int x = 0;
    unsigned int* token_count;
    token_count = &x;
    
    tokenize(buffer, length, words, token_count, token_stream);
    
	free(buffer);

    // keeps track of which element of token_stream we are on
    int element = 0;
    command_stream_t stream = checked_malloc(sizeof(struct command_stream));
    stream->tail = NULL;
    stream->head = NULL;
    stream->cur = NULL;
    bool firstNode = true;
    
    
    while (element < (*token_count))
    {
        command_t command = create_tree(*token_stream, token_count, &element);
        if (command != NULL)
        {
            // If its the first node, then the head is the command, the tail is head
            if (firstNode)
            {
                stream->head = checked_malloc(sizeof(struct command_node));
                stream->head->command = command;
                stream->head->next = NULL;
                stream->tail = stream->head;
                stream->cur = stream->head;
                firstNode = false;
            }
            
            // Else, it sets the next of the tail to a new node, and then sets the command and new tail
            else
            {
                stream->tail->next = checked_malloc(sizeof(struct command_node));
                stream->tail = stream->tail->next;
                stream->tail->command = command;
                stream->tail->next = NULL;
            }
            
        }
    }
    
    return stream;
}



command_t
read_command_stream (command_stream_t s)
{
    // If the cur pointer is null, there are no more nodes to read
    if (s->cur == NULL)
    {
      return NULL;
    }
    
    // Gets the current command, sets the current pointer to the next pointer
    command_t command = s->cur->command;
    s->cur = s->cur->next;
    return command;

}


/////////////////////////////////////////////////////////////////////
/* Functions for help with making the command stream */
/////////////////////////////////////////////////////////////////////

// takes in input of buffer, and produces 2D array of c-string tokens in words
void tokenize (char* buf, int file_length, char** words, unsigned int* wordCount, token** token_stream)
{
    //int wordCount = 0;
    size_t wordSize = 16;
    size_t cmdSize = 3;  //since commands are max 2 chars + \0, only need 3
    size_t streamSize = 256;
    size_t streamLen = streamSize;
    
    words = (char**) checked_malloc(sizeof(char*) * streamSize);
    *token_stream = (token*) checked_malloc((sizeof(token) * streamSize));
    // havent yet implemented realloc for more words/tokens
    
	int i;
    for (i = 0; i < file_length;)
    {
        while (isspace(buf[i]))
        {
            if (*wordCount == 0) // skip through all whitespaces at top
                i++;
            else if (buf[i] != '\n') // skip through non-newlines otherwise
                i++;
            else // it's a newline not at the beginning
                break;
        }
        
        if (isComment(buf[i]))
        {
            while (buf[++i] != '\n')
            { /* skip through the comment*/ }
            // comment up until newline ignored
        }
        
        else if (isValid(buf[i]))
        {   
            size_t currSize = wordSize;
            unsigned int wordLength = 0;
            words[*wordCount] = (char*) checked_malloc(sizeof(char) * wordSize);
            words[*wordCount][wordLength] = buf[i];
            i++;
            
            while (isValid(buf[i]))
            {
                if (wordLength == currSize-2)
                {
                    checked_realloc(words[*wordCount], currSize + wordSize);
                    currSize += wordSize;
                }
                words[*wordCount][++wordLength] = buf[i++];
            }
            words[*wordCount][++wordLength] = '\0';
            
            //(*token_stream)[*wordCount].content = words[*wordCount];
            (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*wordLength);
            strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
            (*token_stream)[*wordCount].type = WORD;
            free(words[*wordCount]);

            (*wordCount)++;
        }
        
        else if (isCmd(buf[i]))
        {
            if (buf[i] == '&')
            {
                if (buf[i+1] == '&')
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = words[*wordCount][1] = '&';
                    words[*wordCount][2] = '\0';
                    i+=2;
		
                    (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                    strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                    free(words[*wordCount]);

                    //(*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = AND;
                    (*wordCount)++;
                }
                else
                {
                    exit_message("Invalid Command. \n");
                }
            }
            
            if (buf[i] == ';')
            {
                if (isCmd(buf[i+1]) && buf[i+1] != '(')
                {
                    exit_message("Invalid Command. \n");
                }
                else
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = ';';
                    words[*wordCount][1] = '\0';
                    i++;

                    (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                    strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                    free(words[*wordCount]);

                   // (*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = SEQ;
                    (*wordCount)++;
                }
            }
            
            if (buf[i] == '|')
            {
                if (buf[i+1] == '|')
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = words[*wordCount][1] = '|';
                    words[*wordCount][2] = '\0';

                    (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                    strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                    free(words[*wordCount]);

                    //(*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = OR;
                    (*wordCount)++, i+=2;
                    
                }
                else if (isCmd(buf[i+1]) && buf[i+1] != '(')
                {
                    exit_message("Invalid Command. \n");
                }
                else // single OR --> PIPE operator
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = '|';
                    words[*wordCount][1] = '\0';

                    (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                    strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                    free(words[*wordCount]);

                   // (*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = PIPE;
                    (*wordCount)++, i++;
                }
            }
            
            if (buf[i] == '(')
            {
                if (isCmd(buf[i+1]) && buf[i+1] != '(' && buf[i+1] != ')')
                {
                    exit_message("Invalid Command. \n");
                }
                else
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = '(';
                    words[*wordCount][1] = '\0';

                    (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                    strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                    free(words[*wordCount]);

                    //(*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = LEFT;
                    (*wordCount)++, i++;
                }
            }
            
            if (buf[i] == ')')
            { // close bracket can be followed by all the cmd operators
                words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                words[*wordCount][0] = ')';
                words[*wordCount][1] = '\0';

                (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                free(words[*wordCount]);

                //(*token_stream)[*wordCount].content = words[*wordCount];
                (*token_stream)[*wordCount].type = RIGHT;
                (*wordCount)++, i++;
            }
            
            if (buf [i] == '<') // can't be followed by &, ;, |, > , <, )
            {
                if (isCmd(buf[i+1]) && buf[i+1] != '(')
                {
                    exit_message("Invalid Command. \n");
                }
                else
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = '<';
                    words[*wordCount][1] = '\0';

                    (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
                    strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
                    free(words[*wordCount]);

                    //(*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = LESS;
                    (*wordCount)++, i++;
                }
            }
            
            if (buf [i] == '>') // can't be followed by &, ;, |, > , <, )
            {
                if (isCmd(buf[i+1]) && buf[i+1] != '(')
                {
                    exit_message("Invalid Command. \n");
                }
                else
                {
                    words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
                    words[*wordCount][0] = '>';
                    words[*wordCount][1] = '\0';

		(*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
		strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);       
		free(words[*wordCount]);  

                    //(*token_stream)[*wordCount].content = words[*wordCount];
                    (*token_stream)[*wordCount].type = GREATER;
                    (*wordCount)++, i++;
                }
            }
        }
        
        else if (buf[i] == '\n')    // save newlines (not at top of file) as its own token
        {
		if (buf[i+1] == '\0')
		{
			i++; break;
		}
            words[*wordCount] = (char*) checked_malloc(sizeof(char) * cmdSize);
            words[*wordCount][0] = '\n';
            words[*wordCount][1] = '\0';

            (*token_stream)[*wordCount].content = (char*) checked_malloc(sizeof(char)*cmdSize);
            strcpy ((*token_stream)[*wordCount].content, words[*wordCount]);
            free(words[*wordCount]);

            //(*token_stream)[*wordCount].content = words[*wordCount];
            (*token_stream)[*wordCount].type = NEWLINE;
            (*wordCount)++, i++;
        }
        else
            i++;
    }
	free(words);
}


command_t
create_tree (token *token_arr, unsigned int *arr_size, int *element)
{
    
    // Flags to indicate whether certain actions need to be performed.
    /* 0: NL, 1: RDIN, 2: RDOUT, 3: SIMPLE, 4: SIMPLE_REQ, 5: RDIN_HANDLED, 6: RDOUT_HANDLED */
    bool flags[NUMFLAGS] =
       {false,  // NL_FLAG
        false,  // RDIN_FLAG
        false,  // RDOUT_FLAG
        false,  // SIMPLE_FLAG
        true,   // SIMPLE_REQ_FLAG
        false,  // RDIN_HANDLED_FLAG
        false,  // RDOUT_HANDLED_FLAG
        false,  // RDOK_FLAG
        false};    // END_FLAG
    
    // Pointer to the previous created command
    command_t prev_command;
    command_t cur_command;
    
    // Current token
    token cur_token;

    // Stack structures to store the operator commands and general commands
    command_stack op_stack;
    command_stack command_stack;
    init(&op_stack);
    init(&command_stack);
    
    // If the final element has been reached, then the function returns NULL
    if (*element >= *arr_size)
        return NULL;
    
    while (*element < *arr_size) {
        cur_token = token_arr[*element];
        *element = *element + 1;
        
        // Allocates to the cur_command space for a command
        cur_command = malloc(sizeof(struct command));           // THIS MUST BE DEALLOCATED BY END
        if (cur_command == NULL)
        {
            exit_message("NO MORE AVAILABLE MEMORY! EXITING. \n");
        }
        cur_command->status = -1;
        cur_command->input = NULL;
        cur_command->output = NULL;
        
        // Handles and sets the flags
        if (handle_flags(cur_token, flags))
        {
            exit_message("Error: Invalid Syntax \n");
            return NULL;
        }
        set_flags(cur_token, flags);
        
        
        // If the END_FLAG is active, adds the fina
        if (flags[END_FLAG])
        {
            cur_command = pop(&command_stack);
            if (cur_command != NULL)
                return cur_command;
            else
            {
                exit_message("Error: Invalid Syntax \n");
                return NULL;
            }
        }
        
        // Create the command
        bool isCommand = false;
        prev_command = pop(&command_stack);
        if (prev_command != NULL)
            push(&command_stack, prev_command);
        isCommand = create_command(cur_token, flags, cur_command, prev_command);
        
        // If it is a command, then adds it to the correct stack and balances the stacks.
        if (isCommand)
        {
            balance_stacks(&command_stack, &op_stack, cur_command);
        }
        
        // If it isn't a command, frees the memory allocated to cur_command
        else
            free(cur_command);
        
    }
    
    if (*element == *arr_size)
    {
        // If there is still a need for a simple flag, it exits with an error.
        if (flags[SIMPLE_REQ_FLAG])
        {
            exit_message("Error: Invalid Syntax \n");
            return NULL;
        }
        
        cur_command = pop(&op_stack);
        command_t command0;
        command_t command1;
        while (cur_command != NULL) {
            
            // If a parenthetical is not closed, then it will throw an error & exit
            if (cur_command->type == LEFT_PARENTHETICAL || cur_command->type == RIGHT_PARENTHETICAL)
            {
                exit_message("Error: Invalid Syntax \n");
                return NULL;
            }
            
            command1 = pop(&command_stack);
            command0 = pop(&command_stack);
            if (command0 != NULL)
            {
                cur_command->u.command[0] = command0;
                cur_command->u.command[1] = command1;
            }
            else
            {
                cur_command->u.command[0] = command1;
            }
            push(&command_stack, cur_command);
            cur_command = pop(&op_stack);
        }
        cur_command = pop(&command_stack);
        if (pop(&command_stack) == NULL)
            return cur_command;
        else
        {
            exit_message("Error: Invalid Syntax \n");
            return NULL;
        }
    }
    
    return NULL;
    
}

// Creates a command using the tokens
bool
create_command(token token, bool *flags, command_t command, command_t prev_command)
{
    switch (token.type) {
            
        // If a simple command isn't required, then it falls through to a SEQ command.
        case NEWLINE:
            if (flags[SIMPLE_REQ_FLAG])
                return false;
        case SEQ:
            command->type = SEQUENCE_COMMAND;
            command->precedence = 1;
            break;
            
        // Simple sets the command
        case OR:
            command->type = OR_COMMAND;
            command->precedence = 2;
            break;
        case AND:
            command->type = AND_COMMAND;
            command->precedence = 2;
            break;
        case PIPE:
            command->type = PIPE_COMMAND;
            command->precedence = 3;
            break;
            
            // Psuedocommands, place holders for full subshell command
        case LEFT:
            command->type = LEFT_PARENTHETICAL;
            command->precedence = -1;
            break;
        case RIGHT:
            command->type = RIGHT_PARENTHETICAL;
            command->precedence = 0;
            break;
            
        case WORD:
            
            // If there is a redirect in/out, redirects and sets flags.
            if (flags[RDIN_FLAG])
            {
                flags[RDIN_FLAG] = false;
                flags[RDIN_HANDLED_FLAG] = true;
                prev_command->input = token.content;
                return false;
            }
            else if (flags[RDOUT_FLAG])
            {
                flags[RDOUT_FLAG] = false;
                flags[RDOUT_HANDLED_FLAG] = true;
                prev_command->output = token.content;
                return false;
            }
            
            // If the simple flag is active, then add words as arguments
            else if (flags[SIMPLE_FLAG])
            {
                prev_command->numWords++;
                prev_command->u.word = checked_realloc(prev_command->u.word, (prev_command->numWords+2)*sizeof(char*));
                prev_command->u.word[prev_command->numWords] = token.content;
                prev_command->u.word[prev_command->numWords+1] = NULL;
                return false;
            }
            
            // If there are no active flags, then it creates a simple command
            else
            {
                flags[SIMPLE_FLAG] = true;
                command->type = SIMPLE_COMMAND;
                command->numWords = 0;
                command->u.word = checked_malloc(sizeof(char*)*2);
                command->u.word[0] = token.content;
                command->u.word[1] = NULL;
            }
            break;
            
        default: return false;
    }
    
    // If it is a command, it will fall through and return true
    return true;

}

// Returns true if there is an error, false if otherwise.
bool
handle_flags(token token, bool *flags)
{
    
    // If there is a second newline, then breaks without error
    if (flags[NL_FLAG] && token.type == NEWLINE)
        return false;
    
    // If there is a simple_req and a word does not come up -> error
    if (flags[SIMPLE_REQ_FLAG] && !(token.type == WORD || token.type == NEWLINE || token.type == LEFT))
        return true;
    
    // If there is a redirect but no word comes up -> error
    if ((flags[RDIN_FLAG] || flags[RDOUT_FLAG]) && token.type != WORD)
        return true;
    
    // If there has already been an RD and an RD comes up -> error
    if (flags[RDIN_HANDLED_FLAG] && token.type == LESS)
        return true;
    if (flags[RDOUT_HANDLED_FLAG] && token.type == GREATER)
        return true;
    
    // If RD is not valid at current time and an RD comes up -> error
    if (!flags[RDOK_FLAG] && (token.type == LESS || token.type == GREATER))
        return true;
    
    if (!flags[SIMPLE_FLAG])
    {
        switch (token.type) {
            case RIGHT:
            case PIPE:
            case SEQ:
            case OR:
            case AND:
                return true;
                break;
                
            default:
                return false;
                break;
        }
    }
    
    return false;
}

// Sets the correct flags
void
set_flags(token token, bool *flags)
{
    
    // Flags that must be set to false every time:
    if (token.type != NEWLINE)
        flags[NL_FLAG] = false;
    
    switch (token.type) {
        
        // RIGHT is a special case because it is OK to RD in/out of subshell
        case RIGHT:
            flags[RDOK_FLAG] = true;                    // It is OK to RD into a subshell
            flags[SIMPLE_FLAG] = false;
            flags[RDIN_HANDLED_FLAG] = false;
            flags[RDOUT_HANDLED_FLAG] = false;
            break;
        
        
        case LEFT:
        case PIPE:
        case SEQ:
        case OR:
        case AND:
            flags[RDOK_FLAG] = false;                   // It isn't OK to RD
            flags[SIMPLE_FLAG] = false;                 // The command isn't simple
            flags[SIMPLE_REQ_FLAG] = true;              // A simple command is required after this command
            flags[RDIN_HANDLED_FLAG] = false;           // There are no handled redirections
            flags[RDOUT_HANDLED_FLAG] = false;
            break;

        // If there are two new lines in a row, then it sets the end flag. If not, just sets
        // the NL Flag
        case NEWLINE:
            if (flags[NL_FLAG] && !flags[SIMPLE_REQ_FLAG])
                flags[END_FLAG] = true;                 // If there are 2 NLs, then set END
            flags[NL_FLAG] = true;
            flags[SIMPLE_FLAG] = false;
            break;
            
        // Redirect cases. Sets the RD flags, sets a requirement for a simple command
        case LESS:
            flags[SIMPLE_REQ_FLAG] = true;
            flags[RDIN_FLAG] = true;
            flags[RDOK_FLAG] = false;
            break;
    
        case GREATER:
            flags[SIMPLE_REQ_FLAG] = true;
            flags[RDOUT_FLAG] = true;
            flags[RDOK_FLAG] = false;
            break;

        // Cases where there is a WORD
        case WORD:
            // If there is a word, there is no requirement of a simple command
            flags[SIMPLE_REQ_FLAG] = false;
            flags[RDOK_FLAG] = true;
            break;
    }
    
}

void
balance_stacks(command_stack *cmdstack, command_stack *opstack, command_t new_command)
{
    // If the command is a simple command, then push it to the cmdstack
    if (new_command->type == SIMPLE_COMMAND)
    {
        push(cmdstack, new_command);
        return;
    }
    
    // If its a left parenthetical, push it to the op stack
    if (new_command->type == LEFT_PARENTHETICAL)
    {
        push(opstack, new_command);
        return;
    }

    // transfer variables
    command_t command0;
    command_t command1;
    command_t old_op = pop(opstack);
    
    // If the new command is of less or equal precedence than operation on stack, pop operation
    while (old_op != NULL && new_command->precedence <= old_op->precedence)
    {
        
        command1 = pop(cmdstack);
        command0 = pop(cmdstack);
        old_op->u.command[1] = command1;
        old_op->u.command[0] = command0;
        push(cmdstack, old_op);
        old_op = pop(opstack);
    }
    
    // If it is a parenthetical, then turn into subshell and put into the command stack.
    if (old_op != NULL && old_op->type == LEFT_PARENTHETICAL && new_command->type == RIGHT_PARENTHETICAL)
    {
        free(old_op);
        new_command->type = SUBSHELL_COMMAND;
        new_command->u.subshell_command = pop(cmdstack);
        push(cmdstack, new_command);
    }
    // If there are remaining operations, put the popped operation back & insert new operation.
    else if (old_op != NULL)
    {
        push(opstack, old_op);
        push(opstack, new_command);
    }
    else
        push(opstack, new_command);
}


// Error with variable message.
void
exit_message(const char* message)
{
    // fprintf(stderr, "%s", message);
    error(1, 0, "%s", message);
    
    exit(EXIT_FAILURE);
    
}


/////////////////////////////////////////////////////////////////////
/* Command Stack Implementation */
/////////////////////////////////////////////////////////////////////

// initializes the stack
void
init(command_stack *stack)
{
    stack->size = 0;
}

void
push(command_stack *stack, command_t command)
{
    // If the stack is empty, allocates fresh memory
    if (stack->size == 0)
    {
        stack->size++;
        stack->stack = (command_t*) checked_malloc(sizeof(command_t)*stack->size);
    }
    
    // Else, it increments the size and then reallocates the memory
    else
    {
        stack->size++;
        stack->stack = (command_t*) checked_realloc((void *)stack->stack, stack->size*sizeof(command_t));
    }
    
    // Sets the last element equal to the command
    stack->stack[stack->size - 1] = command;
    
    return;
}

command_t
pop(command_stack *stack)
{
    // If there are no elements, returns NULL
    if (stack->size == 0)
        return NULL;
    
    // Grabs the highest indexed element of the stack
    command_t command = stack->stack[stack->size - 1];
    stack->size--;
    
    // Reallocates or deletes the memory of the stack based on the new size
    if (stack->size == 0)
        free(stack->stack);
    else
        stack->stack = (command_t*) checked_realloc((void *)stack->stack, stack->size*sizeof(command_t));
    
    // Returns highest indexed element.
    return command;
}
