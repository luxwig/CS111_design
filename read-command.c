#define UNUSED(X) (void)(X)
#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <stdio.h>

//error hanlde
void errorHandle(size_t line)
{
  fprintf(stderr, "Incorrect syntax: line %zu \n", line);
  exit(EXIT_FAILURE);
}
// declaration of command_stream
struct command_stream
{
  command_t* m_command;
  command_t* p_current;
  size_t size;
  size_t capacity;
};

// declaration of elements
struct elements
{
  char* data;
  bool is_operator;
  bool is_sub;
};

//stack for elements
struct stack
{
  size_t size;
  size_t capacity;
  struct elements** c;
};
struct elements* top(struct stack* s)
{
  if (s->size == 0) return NULL;
  return (s->c[s->size - 1]);
}
struct elements* pop(struct stack* s)
{
  if (s->size == 0) return NULL;
  struct elements* t = top(s);
  s->size--;
  return t;
}
void free_stack(struct stack* s)
{
  while (top(s)) { pop(s); }
  free(s->c);
}
void push(struct stack* s, struct elements *e)
{
  s->c[s->size] = e;
  s->size++;
  if (s->size >= s->capacity){
    s->capacity *= 2;
    s->c = checked_grow_alloc(s->c, &(s->capacity));
  }
}
void init(struct stack* s)
{
  s->size = 0;
  s->capacity = 64;
  s->c = checked_malloc(sizeof(struct elements*) * 64);
}

//stack for cmd
struct cmd_stack
{
  size_t size;
  size_t capacity;
  command_t* c;
};
command_t cmd_top(struct cmd_stack* s)
{
  if (s->size == 0) return NULL;
  return (s->c[s->size - 1]);
}
command_t cmd_pop(struct cmd_stack* s)
{
  command_t a;
  if (s->size == 0) return NULL;
  a = cmd_top(s);
  s->size--;
  return a;
}
void free_cmd_stack(struct cmd_stack* s)
{
  while (cmd_top(s)) { cmd_pop(s); }
  free(s->c);
}
void cmd_push(struct cmd_stack* s, command_t e)
{
  s->c[s->size] = e;
  s->size++;
  if (s->size >= s->capacity){
    s->capacity *= 2;
    s->c = checked_grow_alloc(s->c, &(s->capacity));
  }
}
void cmd_init(struct cmd_stack* s)
{
  s->size = 0;
  s->capacity = 64;
  s->c = checked_malloc(sizeof(command_t)* 64);
}

//stack for char
struct cstack
{
  size_t size;
  size_t capacity;
  char* c;
};
char cstack_top(struct cstack *s)
{
  if (s->size == 0) return '\0';
  return (s->c[s->size - 1]);
}
char cstack_pop(struct cstack* s)
{
  char c;
  if (s->size == 0) return '\0';
  c = cstack_top(s);
  s->size--;
  return c;
}
void free_cstack(struct cstack* s)
{
  while (cstack_top(s)) { cstack_pop(s); }
  free(s->c);
};
void cstack_push(struct cstack*s, char c)
{
  s->c[s->size] = c;
  s->size++;
  if (s->size >= s->capacity){
    s->capacity *= 2;
    s->c = checked_grow_alloc(s->c, &(s->capacity));
  }
}
void cstack_init(struct cstack* s)
{
  s->size = 0;
  s->capacity = 64;
  s->c = checked_malloc(sizeof(char)* 64);
}

//check if it is ordinary words
bool is_ordinary(char c)
{
  if (isalpha(c) || isdigit(c))
    return true;
  switch (c)
    {
    case '!':
    case '%':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case '@':
    case '^':
    case '_':
      return true;
    default:
      return false;
    }
}
//check if it is special token including space and newline
bool is_nonordi(char c)
{
  switch (c)
    {
    case '#':
    case '>':
    case'<':
    case '(':
    case ')':
    case '|':
    case '&':
    case ';':
    case ' ':
    case '\t':
    case '\n':
      return true;
    default:
      return false;
    }
}

// cstr copy function
void c_strcpy(char* dest, char* src)
{
  char* tmp = checked_malloc(sizeof(char)* strlen(src) + 1);
  strcpy(tmp, src);
  tmp[strlen(src)] = 0;
  strcpy(dest, tmp);
  dest[strlen(tmp)] = 0;
  free(tmp);//MEMLEAK
}
char* c_strncpy(char* dest, char* src_str, char* src_end)
{
  if (src_end < src_str || !src_str || !src_end) {
    free(dest);
    return NULL;
  }
  int size = (src_end - src_str) + 1;
  char* tmp = checked_malloc(sizeof(char)* (size + 1));
  strncpy(tmp, src_str, size);
  tmp[size] = 0;
  free(dest);
  return tmp;
}

// str check function
bool is_special(char *str)
{
  if (str == NULL) return false;
  char c = str[0];
  return (c == '(' || c == '|' || c == '&' || c == ')' || c == ';');
}
bool is_space(char *str)
{
  if (!str) return NULL;
  if (str == NULL) return false;
  char c = str[0];
  return (c == ' ' || c == '\t' || c == '\n');
}


char* get_first_none_space(char* str)
{
  if (!str) return NULL;
  int size = strlen(str);
  int i;
  for (i = 0; i < size; i++)
    if (str[i] != '\t' && str[i] != ' ' && str[i] != '\n') return str + i;
  return NULL;
}
char* get_last_none_space(char*str)
{
  size_t size = strlen(str), i;
  for (i = size - 1; i > 0; i--)
    if (str[i] != '\t' && str[i] != ' ' && str[i] != '\n') return str + i;
  return str;
}

//return pointer to special character
char* get_special(char* str)
{
  if (!str) return NULL;
  char const d_label[][3] = { "&&", ";", "||", "|", "(", ")" };
  char *dptr = NULL,
    *tmp;
  int i;
  for (i = 0; i < 6; i++)
    {
      tmp = strstr(str, d_label[i]);
      if (!tmp) continue;
      if (dptr)
	dptr = dptr>tmp ? tmp : dptr;
      else
	dptr = tmp;
    }
  return dptr ? dptr : NULL;
}

char* get_special_str(char* str) // return new cstr
{
  if (!str) return NULL;
  char* d = checked_malloc(sizeof(char)* 3);
  if (str[0] == '(' || str[0] == ')' || str[0] == ';') {
    d[0] = str[0]; d[1] = 0;
  }
  else if (str[0] == '|')
    {
      d[0] = str[0];
      d[1] = str[1] == '|' ? '|' : 0;
      d[2] = 0;
    }
  else if (str[0] == '&')
    {
      if (str[1] == '&') { d[0] = '&'; d[1] = '&'; d[2] = 0; }
      else { free(d); return NULL; }
    }
  else free(d);
  return d;
}

// get main func, input and output
char** get_func(char* str)
{
  char* func = checked_malloc(sizeof(char)* (strlen(str) + 1));
  strcpy(func, str);
  char* r1 = strchr(str, '<');
  char* r2 = strchr(str, '>');
  char *p1 = get_first_none_space(str),
    *p2 = get_last_none_space(str);
  if (!(p1&&p2)) return NULL;
  if (r1 || r2)
    {
      r1 = r1 ? r1 : r2;
      if (r1 - p1 <= 0) return NULL;
      strncpy(func, p1, r1 - p1);
      func[r1 - p1] = 0;
      func = c_strncpy(func, func, get_last_none_space(func));
    }
  else {
    strncpy(func, p1, p2 - p1 + 1);
    func[p2 - p1 + 1] = 0;
  }
  size_t size = strlen(func);
  size_t i = 0,
    p = 0,
    cp = 0;
  char* t;
  bool b = true;
  char** r = checked_malloc(sizeof(char*)* (strlen(str) + 1));
  for (i = 0; i < size; i++)
    {
      if (b && (func[i] == ' ' || func[i] == '\t'))
	{
	  t = c_strncpy(NULL, func + p, func + i - 1);
	  r[cp] = t;
	  cp++;
	  b = false;
	  continue;
	}
      if (!b && (func[i] != ' ' && func[i] != '\t'))
	{
	  p = i;
	  b = true;
	}
    }
  t = c_strncpy(NULL, func + p, func + i - 1);
  r[cp] = t;
  cp++;
  r[cp] = NULL;
  free(func);
  if (strchr(t, '&') != NULL) return NULL;
  return r;
}

char* get_input(char *str, bool* valid)
{
  char* func = checked_malloc(sizeof(char)* (strlen(str) + 1));
  strcpy(func, str);
  char* r1 = strchr(str, '<');
  char* r2 = strchr(str, '>');
  if (r1)
    {
      r2 = r2 ? r2 - 1 : str + strlen(str) - 1;
      if (r2 > r1)
	{
	  strncpy(func, r1 + 1, r2 - r1);
	  func[r2 - r1] = 0;
	  func = c_strncpy(func, get_first_none_space(func), get_last_none_space(func));
	}
      else
	{
	  func = NULL;
	}
      if (valid)
	*valid = !(!func || strchr(func, '<') || strchr(func, '>') || !get_first_none_space(func));
      return func;
    }
  if (valid) *valid = true;
  return NULL;
}

char* get_output(char *str, bool* valid)
{
  char* func = checked_malloc(sizeof(char)* (strlen(str) + 1));
  strcpy(func, str);
  char* r1 = strchr(str, '>');
  char* r2 = str + strlen(str) - 1;
  if (r1)
    {
      if (r2 > r1){
	strncpy(func, r1 + 1, r2 - r1);
	func[r2 - r1] = 0;
	func = c_strncpy(func,
			 get_first_none_space(func),
			 get_last_none_space(func));
      }
      else func = NULL;
      if (valid)
	*valid = !(!func ||
		   strchr(func, '<') || strchr(func, '>') ||
		   !get_first_none_space(func));
      return func;
    }
  if (valid) *valid = true;
  return NULL;
}

int get_precedence(char* op)
{
  if (op[0] == '(') return -1;
  if (op[0] == ';') return 0;
  if (op[1] == '&' || op[1] == '|') return 1;
  return 2;
}

enum command_type get_type(char *op)
{
  if (op[0] == '(') return SUBSHELL_COMMAND;
  if (op[0] == ';') return SEQUENCE_COMMAND;
  if (op[1] == '&') return AND_COMMAND;
  if (op[1] == '|') return OR_COMMAND;
  return PIPE_COMMAND;
}

command_t create_cmd(struct elements* e, command_t op1, command_t op2)
{
  command_t r = checked_malloc(sizeof(struct command));
  //if subshell command
  if (get_type(e->data) == SUBSHELL_COMMAND)
    {
      r->type = SUBSHELL_COMMAND;
      r->status = -1;
      r->u.subshell_command = op1;
      bool b;
      char** t = get_func(e->data);
      if (t == NULL || strlen(t[0]) != 1 || t[1] != NULL)
	return NULL;
      //check I/O redirect for shell cmd
      r->output = get_output(e->data, &b);
      if (!b) return NULL;
      r->input = get_input(e->data, &b);
      if (!b) return NULL;
    }
  //operator needs two operands
  else if (e->is_operator)
    {
      r->type = get_type(e->data);
      r->status = -1;
      r->u.command[0] = op1;
      r->u.command[1] = op2;
      r->output = NULL;
      r->input = NULL;
    }
  //for simple cmd
  else
    {
      bool b = false;
      r->type = SIMPLE_COMMAND;
      r->status = -1;
      //get word and check
      r->u.word = get_func(e->data);
      if (!(r->u.word)) return NULL;
      //get input and check
      r->input = get_input(e->data, &b);
      if (!b) return NULL;
      //get output and check
      r->output = get_output(e->data, &b);
      if (!b) return NULL;
    }
  return r;
}

bool isLetterOrCloseParen(char ch)
{
  return ch == 'T' || ch == ')';
};

int precedence(char ch)
{
  if (ch == '(') return 0;
  if (ch == ';') return 1;
  if (ch == '&') return 2;
  else  return 3;
}

//check syntax
bool check(struct elements*e, size_t size)
{
  //create a string tmp that simplifies the string
  char * tmp = checked_malloc(sizeof(char)*(size * 2 + 1));
  size_t t = 0;
  size_t ele;
  for (ele = 0; ele < size; ele++)
    {
      if (!e[ele].is_operator) tmp[t] = 'T';
      else if (e[ele].data[0] == ')' && !is_ordinary(e[ele].data[1])) tmp[t] = ')';
      else if (e[ele].data[0] == ')' && is_ordinary(e[ele].data[1]))
	{
	  tmp[t] = ')';
	  t++;
	  tmp[t] = 'T';
	}
      else if (e[ele].data[0] == '(') tmp[t] = '(';
      else if (e[ele].data[0] == ';') tmp[t] = ';';
      else if (e[ele].data[1] == '&' || e[t].data[1] == '|') tmp[t] = '&';
      else tmp[t] = '|';
      t++;
    }
  tmp[t] = 0;
  //create the postfix string and initiate a stack for char
  char* postfix = checked_malloc(sizeof(char)*(size * 2 + 1));
  size_t i = 0;
  struct cstack s;
  cstack_init(&s);
  char prevch = '(';
  for (t = 0; t < strlen(tmp); t++)
    {
      char ch = tmp[t];
      switch (ch)
	{
	case 'T':
	  if (isLetterOrCloseParen(prevch))
	    return false;
	  postfix[i] = ch;
	  i++;
	  break;
	case'(':
	  if (isLetterOrCloseParen(prevch))
	    return false;
	  cstack_push(&s, ch);
	  break;
	case ')':
	  if (!isLetterOrCloseParen(prevch) || prevch == '(')
	    return false;
	  for (;;)
	    {
	      if (s.size == 0)
		return false;
	      char c = cstack_top(&s);
	      cstack_pop(&s);
	      if (c == '(')
		break;
	      postfix[i] = c;
	      i++;
	    }
	  break;
	case '|':
	case'&':
	case';':
	  if (!isLetterOrCloseParen(prevch))
	    return false;
	while (s.size != 0 && precedence(ch) <= precedence(cstack_top(&s)))
	  {
	    postfix[i] = cstack_top(&s);
	    i++;
	    cstack_pop(&s);
	  }
	cstack_push(&s, ch);
	break;
	default:
	  return false;
	}
      prevch = ch;
    }
  //end of expression; pop remaining operators
  if (!isLetterOrCloseParen(prevch))
    return false;
  while (s.size != 0)
    {
      char c = cstack_top(&s);
      cstack_pop(&s);
      if (c == '(')
	return false;
      postfix[i] = c;
      i++;
    }
  postfix[i] = 0;

  //empty expression
  if (strlen(postfix) == 0)
    return false;

  //free memory
  free_cstack(&s);
  free(postfix);
  free(tmp);
  return true;

};

command_t str_to_cmd(char* str, size_t line)
{
  //get the string
  char* tmp = checked_malloc(sizeof(char)* (strlen(str) + 1));
  strcpy(tmp, str);
  tmp[strlen(str)] = 0;

  //try to grab elements
  size_t size = 0;
  struct elements* e = checked_malloc(sizeof(struct elements) * (strlen(str) + 1));
  char* d = get_special(tmp);
  while (d)
    {
      //try to get operator
      if (d == get_first_none_space(tmp))
	{
	  // get ) and the next operator
	  if (d[0] == ')')
	    {
	      d = get_special(d + 1);
	      e[size].data = c_strncpy(NULL, tmp, d ? d - 1 : strlen(tmp) + tmp);
	    }
	  // get that operator or (
	  else
	    {
	      e[size].data = get_special_str(d);
	      d = d + strlen(e[size].data);
	    }
	  e[size].is_operator = true;
	  e[size].is_sub = false;
	}

      // try to get the operand
      else
	{
	  e[size].data = c_strncpy(NULL, tmp, d - 1);
	  e[size].is_operator = false;
	  e[size].is_sub = false;
	}
      if (!e[size].data) return NULL;

      tmp = c_strncpy(tmp, d, get_last_none_space(tmp));
      d = get_special(tmp);
      size++;
    }

  //get the remaining operand
  if (get_first_none_space(tmp))
    {
      e[size].data = c_strncpy(NULL, tmp, get_last_none_space(tmp));
      e[size].is_operator = false;
      e[size].is_sub = false;
      size++;
    }
  //detect syntax error
  bool detect = check(e, size);
  if (detect == false)
    {
      errorHandle(line);
    }

  //free memory
  free(tmp);

  //initiate stack for elements
  struct stack s;
  init(&s);
  //create an array of pointers to elements called output to store postfix elements
  size_t o_p = 0;
  struct elements** output = checked_malloc(sizeof(struct elements*) * size);
  size_t i = 0;
  for (i = 0; i < size; i++)
    {
      // if it is a operand
      if (!e[i].is_operator)
	{
	  output[o_p] = e + i;
	  o_p++;
	  continue;
	}
      // if it is a (
      if (e[i].data[0] == '(') { push(&s, e + i); continue; }
      // if it is an op and the stack is empty
      if (!top(&s)) { push(&s, e + i); continue; }
      // if it is )
      if (e[i].data[0] == ')')
	{
	  while (top(&s)->data[0] != '(')
	    {
	      output[o_p] = pop(&s);
	      o_p++;
	    }
	  output[o_p] = pop(&s);
	  strcat(output[o_p]->data, e[i].data + 1);
	  o_p++;
	  continue;
	}
      // if it is an op and the stack is not empty
      int pc = get_precedence(e[i].data);
      while (top(&s) && get_precedence(top(&s)->data) >= pc)
	{
	  output[o_p] = pop(&s);
	  o_p++;
	}
      push(&s, e + i);
    }
  while (top(&s))
    {
      output[o_p] = pop(&s);
      o_p++;
    }
  //free memory for stack for elements
  free_stack(&s);


  command_t cmd;
  //initiate stack for cmd
  //officially create cmd
  struct cmd_stack cs;
  cmd_init(&cs);
  for (i = 0; i < o_p; i++)
    {
      //if operator needs two operands
      if (output[i]->is_operator && output[i]->data[0] != '(')
	{
	  command_t op1, op2;
	  op2 = cmd_pop(&cs);
	  op1 = cmd_pop(&cs);
	  if (!op1 || !op2)
	    {
	      return NULL;
	    }
	  cmd = create_cmd(output[i], op1, op2);
	  cmd_push(&cs, cmd);
	  continue;
	}
      //if subshell cmd
      if (output[i]->data[0] == '(')
	{
	  command_t op1;
	  if ((op1 = cmd_pop(&cs)))
	    {
	      cmd = create_cmd(output[i], op1, NULL);
	      cmd_push(&cs, cmd);
	      continue;
	    }
	  else
	    {
	      return NULL;
	    }
	}
      //simple cmd
      cmd = create_cmd(output[i], NULL, NULL);
      cmd_push(&cs, cmd);
      if (!cmd)
	{
	  return NULL;
	}
    }
  command_t rr = cmd_pop(&cs);
  if (cmd_top(&cs) != NULL)
    {
      return NULL;
    }

  //free all remaining memory
  free_cmd_stack(&cs);
  for (i = 0; i < size; i++)
    free(e[i].data);
  free(e);
  free(output);
  return rr;
}

//commit cmd to cmd stream
void commit_cmd(char* buffer, size_t line, command_stream_t ct)
{
  command_t cmd = str_to_cmd(buffer, line);
  if (!cmd)
    {
      errorHandle(line);
    }
  (ct->m_command)[ct->size] = cmd;
  ct->size++;
  if (ct->size >= ct->capacity)
    {
      ct->capacity *= 2;
      ct->m_command = checked_grow_alloc(ct->m_command, &(ct->capacity));
      ct->p_current = ct->m_command;
    }
}


command_stream_t
make_command_stream(int(*get_next_byte) (void *),
		    void *get_next_byte_argument)
{
  /*the only case when we make string into cmd is when we have the case
    where a /n/n b  or the case where we have remaining string
    till the end of the buffer */
  /*notice that in this case, if the script ends with ;
    we consider it as a syntax error*/

  size_t size = 1024,
    count = 0,
    ln = 0;
  int prant = 0;

  //initialize the buffer
  char *buffer = checked_malloc(size);

  //initialize the cmd stream
  command_stream_t ct = checked_malloc(sizeof(struct command_stream));
  ct->capacity = 64;
  ct->size = 0;
  ct->m_command = checked_malloc(sizeof(command_t)* 64);
  ct->p_current = ct->m_command;

  //start reading the file
  void *fm = get_next_byte_argument;
  char t;
  do{
    t = get_next_byte(fm);
    if (t == EOF || t < 0)
      {
	if (t == EOF && get_last_none_space(buffer)[0] == ';')
	  {
	    *get_last_none_space(buffer) = ' ';
	  }
	break;
      }
    if (is_ordinary(t) == 0 && is_nonordi(t) == 0 && t != EOF)
      {
	errorHandle(ln);
      }
    if (t == ')'&& get_last_none_space(buffer)[0] == ';')
      {
	*get_last_none_space(buffer) = ' ';
      }
    //check syntax error for # and delete all useless char
    if (t == '#')
      {
	if (count > 0 && is_ordinary(buffer[count - 1]) == 1)
	  {
	    errorHandle(ln);
	  }
	else
	  {
	    while ((t != EOF && t >= 0) && t != '\n')
	      t = get_next_byte(fm);
	  }
      }
    //case when it is a newline character
    if (t == '\n')
      {
	//no previously open or close prant
	if (prant == 0)
	  {
	    //case when first char is newline, ignore it
	    if (count == 0)
	      {
		ln++;
		continue;
	      }
	    // case when a \n\n b, make previous string into cmd and restart the buffer
	    if (!is_special(get_last_none_space(buffer)) && buffer[count - 1] == '\n')
	      {
		buffer[count - 1] = 0;
		commit_cmd(buffer, ln, ct);
		count = 0;
		ln++;
		continue;
	      }
	    // case when a&& \n b, ignore the newline
	    else if (is_special(get_last_none_space(buffer)) && (*get_last_none_space(buffer)) != ')')
	      buffer[count] = ' ';
	    // otherwise just read the newline
	    else buffer[count] = '\n';
	  }
	//if has previously open prant, ignore newline
	if (prant > 0)
	  {
	    buffer[count] = ' ';
	  }
	// if has previously close prant, error
	if (prant < 0)
	  {
	    errorHandle(ln);
	  }
	ln++;
      }
    //when it is not newline char
    else
      {
	//case when a \n b, change newline char to ;
	if (count > 0 && buffer[count - 1] == '\n')
	  buffer[count - 1] = ';';
	//otherwise just save the char
	buffer[count] = t;
	//check if it is token that can be accepted
	if (t == '(') prant++;
	if (t == ')') prant--;
      }
    count++;
    //reallocate if out of boundary
    if (count >= size)
      {
	size *= 2;
	buffer = checked_grow_alloc(buffer, &size);
      }
    //make the remaining into a string
    buffer[count] = 0;
  } while (t >= 0 && t != EOF);

  //make the remaining string into cmd
  if (count != 0)
    {
      if (prant != 0)
	{
	  errorHandle(ln);
	}
      commit_cmd(buffer, ln, ct);
    }
  ct->p_current = ct->m_command;
  free(buffer);
  return ct;
}


command_t
read_command_stream(command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  if ((s->m_command) + s->size == (s->p_current)) return NULL;
  ((s->p_current))++;
  return *((s->p_current) - 1);
  //error (1, 0, "command reading not yet implemented");
}
