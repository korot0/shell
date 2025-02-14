/*

  Name: Josue Trejo Ruiz
  ID: 1002232581

*/

// The MIT License (MIT)
//
// Copyright (c) 2016 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens
                           // so we need to define what delimits our tokens.
                           // In this case  white space
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

// Increased size to 12 to account for ten arguments.
// We need a size of 12 to account for the command and the NULL terminator
#define MAX_NUM_ARGUMENTS 12 // Mav shell only supports ten arguments

#define PID_HISTORY_SIZE 15
#define COMMAND_HISTORY_SIZE 15

// Circular array structure
typedef struct
{
  pid_t pids[PID_HISTORY_SIZE];
  int count; // Keep track of how many PIDs are stored
} PidHistory;

typedef struct
{
  char commands[COMMAND_HISTORY_SIZE][MAX_COMMAND_SIZE];
  int count;
} CommandHistory;

// Function prototypes
void printPidHistory(PidHistory *ph);
void addPid(PidHistory *ph, pid_t pid);
void printCommandHistory(CommandHistory *ch);
void addCommand(CommandHistory *ch, const char *command);

int main()
{

  PidHistory pidHistory = {.count = 0};
  CommandHistory commandHistory = {.count = 0};

  char *command_string = (char *)malloc(MAX_COMMAND_SIZE);

  while (1)
  {
    // Print out the msh prompt
    printf("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(command_string, MAX_COMMAND_SIZE, stdin))
      ;

    int nthIndex = -1;
    if (command_string[0] == '!')
    {
      char *nthIndexChar = &command_string[1];
      nthIndex = atoi(nthIndexChar);
      if (nthIndex >= 0 && nthIndex <= commandHistory.count)
      {
        strcpy(command_string, commandHistory.commands[nthIndex]);
      }
      else
      {
        printf("Command not in history\n");
        continue;
      }
    }

    /* Parse input */

    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr;

    char *working_string = strdup(command_string);

    // we are going to move the working_string pointer to
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while (((argument_ptr = strsep(&working_string, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

    if (token[0] == NULL || strlen(token[0]) == 0)
    {
      free(head_ptr);
      continue;
    }

    // If the user enters "exit" or "quit", terminate the shell with status 0
    if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
    {
      exit(0);
    }

    // cd command
    if (strcmp(token[0], "cd") == 0)
    {
      if (token[1] == NULL)
      {
        chdir(getenv("HOME"));
      }
      else
      {
        chdir(token[1]);
      }

      addCommand(&commandHistory, command_string);
      continue;
    }

    // pidhistory command
    if (strcmp(token[0], "showpids") == 0)
    {
      addCommand(&commandHistory, command_string);
      printPidHistory(&pidHistory);
      continue;
    }

    // command history command
    if (strcmp(token[0], "history") == 0)
    {
      addCommand(&commandHistory, command_string);
      printCommandHistory(&commandHistory);
      continue;
    }

    pid_t pid = fork(); // Create a child process

    if (pid == -1)
    {
      return -1;
    }

    if (pid == 0) // Child process
    {
      if (execvp(token[0], token) == -1)
      {
        fprintf(stderr, "%s: Command not found.\n", token[0]);
        exit(1);
      }
    }
    else
    {
      if (wait(NULL) == -1)
      {
        continue;
      }
      addPid(&pidHistory, pid);
      addCommand(&commandHistory, command_string);
    }

    free(head_ptr);
  }

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}

void addPid(PidHistory *ph, pid_t pid)
{
  if (ph->count == PID_HISTORY_SIZE)
  {
    for (int i = 1; i < PID_HISTORY_SIZE; i++)
    {
      ph->pids[i - 1] = ph->pids[i];
    }
    ph->count--;
  }
  ph->pids[ph->count] = pid;
  ph->count++;
}

void printPidHistory(PidHistory *ph)
{
  if (ph->count == 0)
  {
    printf("No pid history yet.\n");
    return;
  }

  for (int i = 0; i < ph->count; i++)
  {
    printf("%d: %d\n", i, ph->pids[i]);
  }
}

void addCommand(CommandHistory *ch, const char *command)
{
  if (command == NULL || strcmp(command, "\n") == 0)
  {
    return;
  }

  if (ch->count == COMMAND_HISTORY_SIZE)
  {
    for (int i = 1; i < COMMAND_HISTORY_SIZE; i++)
    {
      strcpy(ch->commands[i - 1], ch->commands[i]);
    }
    ch->count--;
  }
  strncpy(ch->commands[ch->count], command, MAX_COMMAND_SIZE - 1);
  ch->commands[ch->count][MAX_COMMAND_SIZE - 1] = '\0';
  ch->count++;
}

void printCommandHistory(CommandHistory *ch)
{
  for (int i = 0; i < ch->count; i++)
  {
    printf("%d: %s", i, ch->commands[i]);
  }
}
