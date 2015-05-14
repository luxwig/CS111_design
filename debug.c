#include "command.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <error.h>
#include <string.h>
#include <stdio.h>
#include "alloc.h"
bool cvd(char* str)
{
  if (!str) return false;
  return (str[0] == 'h' || str[0] == 'n' || str[0] == 'c' || str[0] == 'q' || str[0] == 'b');
}

bool debugMode()
{
  char str[100]; 
while (1){
  fprintf(stderr, "(debug) ");
  scanf("%100s",str);
  while (strlen(str) != 1 || !cvd(str))
  {
    print_debugInfo("Invalid command. Type \"h\" for help");
    fprintf(stderr, "(debug) ");
    scanf("%100s", str);
  }
  char op = str[0];
  switch (op) {
    case 'n': return true;
    case 'c':
	      print_debugInfo("Continue.");
	      return false;
    case 'q':
	      fprintf(stderr, "(debug) Quit(y/n)? ");
	      scanf("%100s", str);
	      while (strlen(str) != 1 || (str[0]!='y' && str[0] != 'n'))
	      {
		print_debugInfo("Invalid command.");
		fprintf(stderr, "(debug) Quit?(y/n)");
		scanf("%100s", str);
	      }
	      if (str[0] == 'y')
	      {
		print_debugInfo("Quit.");
		exit(0);
	      }
	      else
	      {
		print_debugInfo("Not confirmed.");
		break;
	      }
    case 'b':
	      if (op == 'b')
	      {
		pid_t pid = fork();
		if (pid == 0)
		{
		  char* cmd[2];
		 cmd[0] = "bash";
		 cmd[1] =NULL;
		  execvp(cmd[0], cmd);
		}
		else
		{
		  int status;
		  waitpid(pid, &status, 0);
		  break;
		}
	      }
   default:;
  }
}
}

void print_debugInfo(char* msg)
{
  fprintf(stderr, msg);
  putchar('\n');
}

void print_ec(pid_t pid, int ec)
{
  (void) pid; (void) ec;
}
