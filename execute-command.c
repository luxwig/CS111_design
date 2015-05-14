// UCLA CS 111 Lab 1 command execution
//#define _DEBUG
#define UNUSED(x) (void)(x)
#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <error.h>
#include <string.h>

/*
 * DEBUG INFO:
 *
#ifdef _DEBUG
#include <stdio.h>

void printExam(graphNode* n)
{
  if (!n) return;
  printf("Address %p\n", n);
  if (n->cmdNode->cmd->type != SIMPLE_COMMAND) 
    printf("cmd type: %d\n",n->cmdNode->cmd->type);
  else 
    printf("cmd %s, input %s, output %s\n", n->cmdNode->cmd->u.word[0],
 					n->cmdNode->cmd->input,
					n->cmdNode->cmd->output);
  graphNode ** head =  n->before;

  printf("Depend on :");
  while (head && *head)
  {
    printf("%p\t",*head);
    head++;
  }
  printf("\n\n");
}

void printdep(depGraph* g)
{
  if (!g) return;
  graphNode ** head = g->dep;
  printf("Dependent command:\n");
  while (head && *head)
  {
    printf("%p\t", *head);
    head++;
  }
  printf("\n");
  head = g->ndep;
  printf("Independent command:\n");
  while (head && *head)
  {
    printf("%p\t", *head);
    head++;
  }
  printf("\n");
}
#endif
*/

void exe_cmd(command_t c);

int command_status(command_t c)
{
  return c->status;
}

void exe_redi_cmd(command_t c)
{
  if (c->input != NULL)
    {
      int fd = open(c->input, O_RDONLY);
      if (fd < 0)
	{
	  error(1, 0, "Cannot open file");
	}
      int redi = dup2(fd, 0);
      if (redi < 0)
	{
	  error(127, 0, "Redirect input error");
	}
      close(fd);
    }
  if (c->output != NULL)
    {
      int fd = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0646);
      if (fd < 0)
	{
	  error(1, 0, "Cannot open file");
	}
      int redi = dup2(fd, 1);
      if (redi < 0)
	{
	  error(127, 0, "Redirect output error");
	}
      close(fd);
    }
}

void exe_simple_cmd(command_t c)
{
  char str[] = "exec";
  if (strcmp(c->u.word[0], str) == 0)
    {
      exe_redi_cmd(c);
      execvp(c->u.word[1], &(c->u.word[1]));
      error(127, 0, "Command does not found");
    }
  else
    {
      int pid = fork();

      //cannot fork
      if (pid < 0)
	{
	  error(127, 0, "Forking error");
	}

      //child process
      else if (pid == 0)
	{
	  exe_redi_cmd(c);
	  execvp(c->u.word[0], c->u.word);
	  error(127, 0, "Command does not found");
	}
      //parent process
      else
	{
	  int status;
	  if (waitpid(pid, &status, 0) < 0)
	    {
	      error(127, 0, "waitpid failed");
	    }
	  else
	    {
	      int exitstatus = WEXITSTATUS(status);
	      c->status = exitstatus;
	    }
	}
    }
}

void exe_and_cmd(command_t c)
{
  //execute left part
  exe_cmd(c->u.command[0]);
  //if success, then execute right part
  if (c->u.command[0]->status == 0)
    {
      exe_cmd(c->u.command[1]);
      c->status = c->u.command[1]->status;
    }
  //if not success, do not have to do the right part
  else
    {
      c->status = c->u.command[0]->status;
    }
}

void exe_or_cmd(command_t c)
{
  //execute the left part
  exe_cmd(c->u.command[0]);
  //if success, do not have to execute the right part
  if (c->u.command[0]->status == 0)
    {
      c->status = c->u.command[0]->status;
    }
  //if not success, execute right part
  else
    {
      exe_cmd(c->u.command[1]);
      c->status = c->u.command[1]->status;
    }
}

void exe_sequence_cmd(command_t c)
{
  exe_cmd(c->u.command[0]);
  exe_cmd(c->u.command[1]);
  c->status = c->u.command[1]->status;
}

void exe_pipe_cmd(command_t c)
{
  int fd[2];
  pipe(fd);
  pid_t firstpid = fork();
  if (firstpid == 0)
    {
      close(fd[0]);
      dup2(fd[1], 1);
      exe_cmd(c->u.command[0]);
      exit(c->u.command[0]->status);
    }
  else if (firstpid < 0)
    error(127, 0, "Forking error");
  else
    {
      pid_t secondpid = fork();
      if (secondpid == 0)
	{
	  close(fd[1]);
	  dup2(fd[0], 0);
	  exe_cmd(c->u.command[1]);
	  exit(c->u.command[1]->status);
	}
      else if (secondpid < 0)
	error(127, 0, "Forking error");
      else
	{
	  close(fd[0]);
	  close(fd[1]);
	  int f_status;
	  int s_status;
	  pid_t returnpid = waitpid(-1, &f_status, 0);
	  if (returnpid == firstpid)
	    {
	      //set LHS exit status
	      c->u.command[0]->status = WEXITSTATUS(f_status);
	      //set RHS exit status and pipe cmd exit status
	      waitpid(secondpid, &s_status, 0);
	      c->u.command[1]->status = WEXITSTATUS(s_status);
	      c->status = c->u.command[1]->status;
	    }
	  else if (returnpid == secondpid)
	    {
	      //set RHS exit status and pipe cmd exit status
	      c->u.command[1]->status = WEXITSTATUS(f_status);
	      c->status = c->u.command[1]->status;
	      //set LHS exit status
	      waitpid(firstpid, &s_status, 0);
	      c->u.command[0]->status = WEXITSTATUS(s_status);
	    }
	  else
	    error(127, 0, "waitpid error");
	}
    }
}


void exe_sub_cmd(command_t c)
{
  int pid = fork();

  //cannot fork
  if (pid < 0)
    {
      error(127, 0, "Forking error");
    }

  //child process
  else if (pid == 0)
    {
      exe_redi_cmd(c);
      exe_cmd(c->u.subshell_command);
      exit(c->u.subshell_command->status);
    }
  //parent process
  else
    {
      int status;
      if (waitpid(pid, &status, 0) < 0)
	{
	  error(127, 0, "waitpid failed");
	}
      else
	{
	  int exitstatus = WEXITSTATUS(status);
	  c->status = exitstatus;
	}
    }
}

void exe_cmd(command_t c)
{
  if (!c) error(1, 0, "Command cannot exec");
  switch (c->type)
    {
    case SIMPLE_COMMAND:
      exe_simple_cmd(c);
      break;
    case AND_COMMAND:
      exe_and_cmd(c);
      break;
    case OR_COMMAND:
      exe_or_cmd(c);
      break;
    case SEQUENCE_COMMAND:
      exe_sequence_cmd(c);
      break;
    case SUBSHELL_COMMAND:
      exe_sub_cmd(c);
      break;
    case PIPE_COMMAND:
      exe_pipe_cmd(c);
      break;
    default:
      error(1, 0, "not specified");
    }
}


struct wlist* cmd_parse_output(command_t c);
struct rlist* cmd_parse_input(command_t c);

struct wlist* simple_cmd_parse_output(command_t c)
{
  struct wlist* t = NULL;
  if (c->output != NULL)
    {
      t = checked_malloc(sizeof(struct wlist));
      {
	t->content = c->output;
	t->next = NULL;
      };
    }
  return t;
}

struct rlist* simple_cmd_parse_input(command_t c)
{
  struct rlist* t = NULL;
  if (c->input != NULL)
    {
      t = checked_malloc(sizeof(struct rlist));
      t->content = c->input;
      t->next = NULL;
    }
  {
    int i = 1;
    char* words;
    while ((words = c->u.word[i]) != NULL)
      {
	if (words[0] != '-')
	  {
	    if (t == NULL)
	      {
		t = checked_malloc(sizeof(struct rlist));
		t->content = words;
		t->next = NULL;
	      }
	    else
	      {
		struct rlist* tmp = t;
		t = checked_malloc(sizeof(struct rlist));
		t->content = words;
		t->next = tmp;
	      }
	  }
	i++;
      }
  }
  return t;
}

struct wlist* sub_cmd_parse_output(command_t c)
{
  struct wlist* t = cmd_parse_output(c->u.subshell_command);
  if (c->output != NULL)
    {
      struct wlist* tmp = t;
      t = checked_malloc(sizeof(struct wlist));
      t->content = c->output;
      t->next = tmp;
    }
  return t;
}

struct rlist* sub_cmd_parse_input(command_t c)
{
  struct rlist* t = cmd_parse_input(c->u.subshell_command);
  if (c->input != NULL)
    {
      struct rlist* tmp = t;
      t = checked_malloc(sizeof(struct rlist));
      t->content = c->output;
      t->next = tmp;
    }
  return t;
}

struct wlist* bi_cmd_parse_output(command_t c)
{
  struct wlist* t1 = cmd_parse_output(c->u.command[0]);
  struct wlist* t2 = cmd_parse_output(c->u.command[1]);
  if (t1 == NULL && t2 == NULL) return NULL;
  if (t1 == NULL && t2 != NULL) return t2;
  if (t1 != NULL && t2 == NULL) return t1;
  struct wlist*tmp = t1;
  while (tmp->next != NULL)
    {
      tmp = tmp->next;
    }
  tmp->next = t2;
  return t1;
}

struct rlist* bi_cmd_parse_input(command_t c)
{
  struct rlist* t1 = cmd_parse_input(c->u.command[0]);
  struct rlist* t2 = cmd_parse_input(c->u.command[1]);
  if (t1 == NULL && t2 == NULL) return NULL;
  if (t1 == NULL && t2 != NULL) return t2;
  if (t1 != NULL && t2 == NULL) return t1;
  struct rlist*tmp = t1;
  while (tmp->next != NULL)
    {
      tmp = tmp->next;
    }
  tmp->next = t2;
  return t1;
}


struct wlist* cmd_parse_output(command_t c)
{
  switch (c->type)
    {
    case SIMPLE_COMMAND:
      return simple_cmd_parse_output(c);
    case SUBSHELL_COMMAND:
      return sub_cmd_parse_output(c);
    default:
      return bi_cmd_parse_output(c);
    }
}

struct rlist* cmd_parse_input(command_t c)
{
  switch (c->type)
    {
    case SIMPLE_COMMAND:
      return simple_cmd_parse_input(c);
    case SUBSHELL_COMMAND:
      return sub_cmd_parse_input(c);
    default:
      return bi_cmd_parse_input(c);
    }
}

rwnode* create_rwnode(command_t c)
{
  rwnode* t = checked_malloc(sizeof(rwnode));
  //create read/write list
  t->readlist = cmd_parse_input(c);
  t->writelist = cmd_parse_output(c);
  //create node to store the root node of each cmd tree
  t->cmd = c;
  return t;
}

bool check_RAW(struct wlist* t1, struct rlist* t2)
{
  struct wlist* w = t1;
  struct rlist* r = t2;
  while (r != NULL)
    {
      while (w != NULL)
	{
	  if (strcmp((w->content), (r->content)) == 0)
	    return false;
	  w = w->next;
	}
      r = r->next;
    }
  return true;
}

bool check_WAW(struct wlist* t1, struct wlist* t2)
{
  struct wlist* w1 = t1;
  struct wlist* w2 = t2;
  while (w1 != NULL)
    {
      while (w2 != NULL)
        {
          if (!strcmp((w1->content),(w2->content)))
            return false;
          w2 = w2->next;
        }
      w1 = w1->next;
    }
  return true;
}

bool check_WAR(struct rlist* t1,struct wlist* t2)
{
  struct rlist* r = t1;
  struct wlist* w = t2;
  while (r != NULL)
    {
      while (w != NULL)
        {
          if (strcmp((w->content),(r->content)) == 0)
            return false;
          w = w->next;
        }
      r = r->next;
    }
  return true;

}

bool check_dependency(const rwnode*t1, const rwnode*t2)
{
  struct rlist* r1 = cmd_parse_input(t1->cmd);
  struct wlist* w1 = cmd_parse_output(t1->cmd);
  struct rlist* r2 = cmd_parse_input(t2->cmd);
  struct wlist* w2 = cmd_parse_output(t2->cmd);
  if (!check_RAW(w1, r2)) return false;
  if (!check_WAW(w1, w2)) return false;
  if (!check_WAR(r1, w2)) return false;
  return true;
}



size_t getSize(graphNode **list)
{
  if (!list) return 0;
  size_t s = 0;
  while(*list)
  {
    list++;
    s++;
  }
  return s + 1;
}

graphNode** add(graphNode* list[], graphNode* item)
{
  size_t size = getSize(list);
  if (size == 0)
  {
    size = 2;
    list = checked_malloc(sizeof(graphNode*) * 2);
  }
  else
  {
    size += 1;
    list = checked_realloc(list, sizeof(graphNode*) * size);
  }
  list[size - 1] = NULL;
  list[size - 2] = item; 
  return list;
}

graphNode* createGraphNode(rwnode* node, graphNode** nodeList, int count)
{
  graphNode* ret = checked_malloc(sizeof(graphNode));
  ret->cmdNode = node;
  ret->before = NULL;
  ret->pid = -1;
  graphNode** head = nodeList;
  int i = 0;
  for (i = 0; i < count; i++)
  {
    if (!check_dependency(node, (*head)->cmdNode))
    {
      ret->before = add(ret->before, *head);
    }
    head++;
  }
  return ret;
}

depGraph* createGraph(command_stream_t s)
{
  	size_t count = 0,
	       ndep_c = 0,
	       dep_c = 0;
	command_t c;
	depGraph* ret = checked_malloc(sizeof(depGraph*));
	ret->ndep = checked_malloc(sizeof(graphNode*));
	ret->dep = checked_malloc(sizeof(graphNode*));
	ret->ndep[0] = NULL;
	ret->dep[0] = NULL;
	graphNode** gNode = checked_malloc(sizeof(graphNode*));
	while ((c = read_command_stream(s)))
	{
	  	gNode = checked_realloc(gNode, sizeof(graphNode*) * (count+2));
		rwnode* t = create_rwnode(c);
		gNode[count] = createGraphNode(t,gNode,count);
		if (gNode[count]->before)
		{
		  ret->dep = checked_realloc(ret->dep, sizeof(graphNode*) * (dep_c+2));
		  ret->dep[dep_c] = gNode[count];
		  dep_c++;
		  ret->dep[dep_c] = NULL;
		}
		else
		{
		  ret->ndep = checked_realloc(ret->ndep, sizeof(graphNode*) * (ndep_c+2));
		  ret->ndep[ndep_c] = gNode[count];
		  ndep_c++;
		  ret->ndep[ndep_c] = NULL;
		}
		gNode[count + 1] = NULL;

/*
 * DEBUG INFO:
 *
#ifdef _DEBUG
		printExam(gNode[count]);
#endif
*/

		count++;
	}

/*
 * DEBUG INFO
 *
#ifdef _DEBUG
	printdep(ret);
#endif
*/

        return ret;
}

int executeNoDep(graphNode** c)
{
  int i = 0;
  while (c[i] != NULL)
    {
       pid_t pid = fork();
      if (pid == 0)
      {
	  execute_command(c[i]->cmdNode->cmd, 1);
	  exit(c[i]->cmdNode->cmd->status);
      }
      else
      {
	  c[i]->pid = pid;
      }
      i++;
    }
  return 1;
}

int executeDep(graphNode** c)
{
  int i = 0;
  while (c[i] != NULL)
    {
      int j = 0;
      graphNode* dep = c[i];
      graphNode* prev;
     loop_label:
      while ((prev = dep->before[j])!= NULL)
	{
	  if (prev->pid == -1)
	    goto loop_label;
	  else j++;
	}
      
      j = 0;
      int status;
      while ((prev = dep->before[j]) != NULL)
	{
	  waitpid(prev->pid, &status, 0);
	  j++;
	}
      pid_t pid = fork();
      if (pid == 0)
	{
	  execute_command(dep->cmdNode->cmd, 1);
	  exit(c[i]->cmdNode->cmd->status);
	}
      else
	dep->pid = pid;
      i++;
    }
  return 1;
}

int executeGraph(depGraph* t)
{
  int i = executeNoDep(t->ndep);
  int j = executeDep(t->dep);
  int a;
  int status;
  a = 0;
  while (t->ndep[a] != NULL)
    {
      if (t->ndep[a]->pid > 0)
	waitpid(t->ndep[a]->pid, &status, 0);
      a++;
    }
  a = 0;
  while (t->dep[a] != NULL)
    {
      if (t->dep[a]->pid > 0)
	waitpid(t->dep[a]->pid, &status, 0);
      a++;
    }
  if (i && j) return 1;
  else return 0;
}

void
execute_command(command_t c, bool time_travel)
{
  if (time_travel == 0)
    exe_cmd(c);
  if (time_travel == 1)
    exe_cmd(c);

}
