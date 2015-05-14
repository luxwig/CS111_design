#include <stdio.h>

bool debugMode(void)
{
  char str[100];
  fprintf(stderr, "(debug) ");
  scanf("%100s", str);
  while (strlen(str) != 1)
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
	      fprintf(stderr, "(debug) Quit?(y/n) ");
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
	      }
    case 'b':
	      if (op == 'b')
	      {
		pid_t pid = fork();
		if (pid == 0)
		{
		  char** cmd = {"bash"};
		  execvp(cmd[0], cmd);
		}
		else
		{
		  int status;
		  waitpid(pid, &status, 0);
		}
	      }
  }
  return true;
}


