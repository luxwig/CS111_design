#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage(void)
{
  error(1, 0, "usage: %s [-dptvx] SCRIPT-FILE", program_name);
}

static int
get_next_byte(void *stream)
{
  return getc(stream);
}

/*
void verbose_helper(command_t command)
{  
*/
int
main(int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  bool time_travel = false;
  bool verbose = false;
  bool xbose = false;
  bool doption = false;
  bool _pause = false;
  if (doption) doption = false;
  program_name = argv[0];

  for (;;)
    switch (getopt(argc, argv, "dptvx"))
      {
      case 'p': print_tree = true; break;
      case 't': time_travel = true; break;
      case 'v': verbose = true; break;
      case 'x': xbose = true; break;
      case 'd': doption = true; break;
      default: usage(); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;
  //some options are exclusive
  if (doption == true && (verbose == false && xbose == false))
    usage();
  if (time_travel == true && (verbose == true || xbose == true || doption == true))
    time_travel = false;

  _pause = doption;
  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage();
  script_name = argv[optind];
  FILE *script_stream = fopen(script_name, "r");
  if (!script_stream)
    error(1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream(get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;
  
  if (time_travel) {
    depGraph*graph = createGraph(command_stream);
    int finalstatus = 0;
    if (graph != NULL)
      {
        finalstatus = executeGraph(graph, xbose, doption);
        if (finalstatus == 0) error(1, 0, "error");
      }
    else
      error(127,0,"empty file");
  }
  else
    {
      bool f = false;
      while ((command = read_command_stream(command_stream)))
	{
	  if (print_tree)
	    {
	      printf("# %d\n", command_number++);
	      print_command(command);
	    }
	  else
	    {
	      last_command = command;
	      if (verbose) 
		{
		  if (!f) f = true;
		  else fprintf(stderr, "\n");
		  if (doption) fprintf(stderr, "* ");
		  print_verbose(command);
		  if (_pause) _pause = debugMode();
		}
	      _pause = execute_command(command, time_travel, xbose, doption, _pause);
	      if (doption) print_ec("",-1, command_status(command));
	    }
	}
    }

  return print_tree || !last_command ? 0 : command_status(last_command);
}
