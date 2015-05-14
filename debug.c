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

const char* ANSI_COLOR_GREEN = "\x1b[32m";
const char* ANSI_COLOR_YELLOW = "\x1b[33m";
const char* ANSI_COLOR_BLUE = "\x1b[34m";
const char* ANSI_COLOR_MAGENTA = "\x1b[35m";
const char* ANSI_COLOR_CYAN = "\x1b[36m";
const char* ANSI_COLOR_RESET = "\x1b[0m";
const char* ANSI_COLOR_RED ="\x1b[31m";

void print_debugInfo(char* msg, char* color)
{
  if (!color) 
    {
      color = checked_malloc(sizeof(char) * 100);
      strcpy(color, ANSI_COLOR_RESET);
    }
  char before[500], after[20];
  strcpy(after, " ]");
  strcpy(before, " [ Debug Info: ");
  strcat(before, msg);
  strcat(before,after);
  fprintf(stderr,"%s%s%s\n", color, before, ANSI_COLOR_RESET);
}

void print_ec(char* cmd, pid_t pid, int status)
{
  if (pid != -1)
    {
      fprintf(stderr, "%s     [ + Debug Info: Process - %d\n",(status != 0)?ANSI_COLOR_RED:ANSI_COLOR_GREEN, pid);
      fprintf(stderr, "       +             Command -%s\n", cmd);
      fprintf(stderr, "       +             exited with status %d ]%s\n\n",status,ANSI_COLOR_RESET);}
  else 
    fprintf(stderr, "%s     [ * Debug Info: Command exited with status %d ]%s\n",(status!=0)?ANSI_COLOR_RED:ANSI_COLOR_YELLOW, status, ANSI_COLOR_RESET);
}

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
	print_debugInfo("Invalid command. Type \"h\" for help", NULL);
	fprintf(stderr, "(debug) ");
	scanf("%100s", str);
      }
    char op = str[0];
    switch (op) {
    case 'n': return true;
    case 'c':
      fprintf(stderr, "Continue.");
      return false;
    case 'q':
      fprintf(stderr, "(debug) Quit(y/n)? ");
      scanf("%100s", str);
      while (strlen(str) != 1 || (str[0]!='y' && str[0] != 'n'))
	{
	  print_debugInfo("Invalid command.", NULL);
	  fprintf(stderr, "(debug) Quit?(y/n)");
	  scanf("%100s", str);
	}
      if (str[0] == 'y')
	{
	  fprintf(stderr, "Quit.\n");
	  exit(0);
	}
      else
	{
	  fprintf(stderr, "Not confirmed.\n");
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
    case 'h':
      print_debugInfo("\t\t\t\tHelp File\t\t\t\t\t", NULL);
      print_debugInfo("b : Bash\t\t| Execute bash.\t\t\t\t\t\t",NULL);
      print_debugInfo("c : Continue\t| Continue running the bash with out any interaction.\t", NULL);
      print_debugInfo("h : Help\t\t| Show this help file.\t\t\t\t\t", NULL);
      print_debugInfo("n : Next\t\t| Run next command.\t\t\t\t\t", NULL);
      print_debugInfo("q : Quit\t\t| Quit the bash.\t\t\t\t\t", NULL);
      break;
    default:;
    }
  }
}
