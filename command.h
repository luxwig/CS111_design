// UCLA CS 111 Lab 1 command interface

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;


typedef struct
{
  	command_t cmd;
	struct rlist* readlist;
	struct wlist* writelist;
}rwnode;


typedef struct rwnode* rwnode_t;

struct graphNN
{
	rwnode* cmdNode;
	struct graphNN** before;
	pid_t pid;   //initialize to -1
};
typedef struct graphNN graphNode;

typedef struct graphNode* graphNode_t;
typedef struct
{
	graphNode** ndep;
	graphNode** dep;
}depGraph;

struct rlist
{
	char* content;
	struct rlist* next;
};

struct wlist

{
	char* content;
	struct wlist* next;
};


/* Create a command stream from GETBYTE and ARG.  A reader of
the command stream will invoke GETBYTE (ARG) to get the next byte.
GETBYTE will return the next input byte, or a negative number
(setting errno) on failure.  */
command_stream_t make_command_stream(int(*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
an error, report the error and exit instead of returning.  */
command_t read_command_stream(command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command(command_t);

/* Execute a command.  Use "time travel" if the flag is set.  */
void execute_command(command_t, bool);

/* Return the exit status of a command, which must have previously
been executed.  Wait for the command, if it is not already finished.  */
int command_status(command_t);

int executeGraph(depGraph*);

depGraph* createGraph(command_stream_t);
