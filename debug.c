#include <stdio.h>

bool debugMode(void)
{
  char str[100];
  printf("(debug) ");
  scanf("%100s", str);
  while (strlen(str) != 1)
  {
    print_debugInfo("Invalid command. Type \"h\" for help");
    printf("(debug) ");
    scanf("%100s", str);
  }
  char op = str[0];
  switch (op) {
    case 'n': return true;
    case 'c':
	      print_debugInfo("Continue.");
	      return false;
    case 'q':
	      printf("(debug) Quit?(y/n) ");
	      scanf("%100s", str);
	      while (strlen(str) != 1 || strlen)
	      {
		print_debugInfo("Invalid command.");
		printf("(debug) ");
		scanf("%100s", str);

	      }
  }
  return true;
}


