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

// Circular array structure
typedef struct
{
  pid_t pids[PID_HISTORY_SIZE];
  int count; // Keep track of how many PIDs are stored
} PidHistory;

// Function prototypes
void printPidHistory(PidHistory *ph);
void addPid(PidHistory *ph, pid_t pid);

int main()
{

  PidHistory pidHistory = {.count = 0};

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
      ; // placeholder for until a valid input is read

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

    // // Now print the tokenized input as a debug check
    // // \TODO Remove this code and replace with your shell functionality
    // int token_index = 0;
    // for (token_index = 0; token_index < token_count; token_index++)
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index]);
    // }

    // Ignore blank line
    if (token[0] != NULL)
    {
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
        continue;
      }
      // pidhistory command
      if (strcmp(token[0], "pidhistory") == 0)
      {
        printPidHistory(&pidHistory);
        continue;
      }
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
      addPid(&pidHistory, pid);
      printf("%d\n", pid); // Store pid in circular array
      wait(NULL);
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
    for (int i = 0; i < PID_HISTORY_SIZE; i++)
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