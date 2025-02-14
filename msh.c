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

// Define the size of the process ID history and command history
#define PID_HISTORY_SIZE 15
#define COMMAND_HISTORY_SIZE 15

// Structure to hold process IDs for history
typedef struct
{
  pid_t pids[PID_HISTORY_SIZE];
  int count;
} PidHistory;

// Structure to hold commands for history
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
char *getCommandFromHistory(CommandHistory *history, int index);
int parseHistoryIndex(char *command_string);
void handleHistoryCommand(char *command_string, CommandHistory *history);

int main()
{
  // Initialize PID history and command history
  PidHistory pidHistory = {.count = 0};
  CommandHistory commandHistory = {.count = 0};
  char *command_string = (char *)malloc(MAX_COMMAND_SIZE); // Allocate memory for command input

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

    // Handle history command (e.g., !n)
    if (command_string[0] == '!')
    {
      handleHistoryCommand(command_string, &commandHistory);
      if (command_string[0] == '\0')
        continue;
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];
    int token_count = 0;
    char *argument_ptr; // Pointer to point to the token parsed by strsep
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
        token[token_count] = NULL;
      token_count++;
    }

    // Skip if no valid tokens are found
    if (token[0] == NULL || strlen(token[0]) == 0)
    {
      free(head_ptr);
      continue;
    }

    // Handle 'exit' or 'quit' commands to exit the shell
    if (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)
    {
      exit(0);
    }
    else if (strcmp(token[0], "cd") == 0) // Handle 'cd' command to change directories
    {
      if (token[1] == NULL)
        chdir(getenv("HOME"));
      else
        chdir(token[1]);
      addCommand(&commandHistory, command_string); // Add to command history
      continue;
    }
    else if (strcmp(token[0], "showpids") == 0) // Handle 'showpids' command to display PID history
    {
      addCommand(&commandHistory, command_string); // Add to command history
      printPidHistory(&pidHistory);                // Print PID history
      continue;
    }
    else if (strcmp(token[0], "history") == 0) // Handle 'history' command to display process history
    {
      addCommand(&commandHistory, command_string); // Add to command history
      printCommandHistory(&commandHistory);        // Print command history
      continue;
    }
    else
    {
      // Fork a new child process
      pid_t pid = fork();
      if (pid == -1)
        return -1;

      if (pid == 0) // Child process
      {
        if (execvp(token[0], token) == -1) // Try executing the user command
        {
          printf("%s: Command not found.\n", token[0]);
          exit(1);
        }
        exit(0);
      }
      else
      {
        if (wait(NULL) == -1) // Parent process
          continue;
        addPid(&pidHistory, pid);                    // Add to PID history
        addCommand(&commandHistory, command_string); // Add command to history
      }
    }

    // Free memory for the tokens
    for (int i = 0; i < token_count; i++)
    {
      if (token[i] != NULL)
        free(token[i]);
    }
    free(head_ptr);
  }
  free(command_string);
  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}

/**
 * Adds a PID to the PID history.
 * This function will add a new PID to the history, and remove the oldest one if history is full.
 * It switches element to the left when the array is full.
 *
 * Parameters:
 *  - ph: Pointer to the PidHistory structure
 *  - pid: The process ID to add to the history
 *
 * Returns: void
 */
void addPid(PidHistory *ph, pid_t pid)
{
  if (ph->count == PID_HISTORY_SIZE)
  {
    for (int i = 1; i < PID_HISTORY_SIZE; i++)
      ph->pids[i - 1] = ph->pids[i];
    ph->count--;
  }
  ph->pids[ph->count] = pid;
  ph->count++;
}

/**
 * Prints the history of PIDs.
 * This function prints out all PIDs stored in the PID history.
 *
 * Parameters:
 *  - ph: Pointer to the PidHistory structure
 *
 * Returns: void
 */
void printPidHistory(PidHistory *ph)
{
  if (ph->count == 0)
  {
    printf("No pid history yet.\n");
    return;
  }
  for (int i = 0; i < ph->count; i++)
    printf("%d: %d\n", i, ph->pids[i]);
}

/**
 * Adds a command to the command history.
 * This function adds a new command to the history, and removes the oldest one if history is full.
 *
 * Parameters:
 *  - ch: Pointer to the CommandHistory structure
 *  - command: The command string to add to the history
 *
 * Returns: void
 */
void addCommand(CommandHistory *ch, const char *command)
{
  if (command == NULL || strcmp(command, "\n") == 0)
    return;

  if (ch->count == COMMAND_HISTORY_SIZE)
  {
    for (int i = 1; i < COMMAND_HISTORY_SIZE; i++)
      strcpy(ch->commands[i - 1], ch->commands[i]);
    ch->count--;
  }
  strncpy(ch->commands[ch->count], command, MAX_COMMAND_SIZE - 1);
  ch->commands[ch->count][MAX_COMMAND_SIZE - 1] = '\0';
  ch->count++;
}

/**
 * Prints the history of commands.
 * This function prints out all commands stored in the command history.
 *
 * Parameters:
 *  - ch: Pointer to the CommandHistory structure
 *
 * Returns: void
 */
void printCommandHistory(CommandHistory *ch)
{
  for (int i = 0; i < ch->count; i++)
    printf("%d: %s", i, ch->commands[i]);
}

/**
 * Gets a command from the history by index.
 * This function retrieves the command at the specified index from the command history.
 *
 * Parameters:
 *  - history: Pointer to the CommandHistory structure
 *  - index: The index of the command in history to retrieve
 *
 * Returns: The command string at the specified index, or NULL if the index is out of range
 */
char *getCommandFromHistory(CommandHistory *history, int index)
{
  if (index >= 0 && index < history->count)
    return history->commands[index];
  else
    return NULL;
}

/**
 * Parses the history index from the command string.
 * This function extracts the index of the command from a string like "!n"
 * (where n is the index of a previous command) and converts it to an integer.
 *
 * Parameters:
 *  - command_string: The command string that contains the history index (e.g., "!5")
 *
 * Returns:
 *  - An integer representing the parsed history index (e.g., 5 for "!5").
 */
int parseHistoryIndex(char *command_string)
{
  char *indexPart = &command_string[1];
  return atoi(indexPart);
}

/**
 * Handles the history command (e.g., "!n") and retrieves the correspond command
 * from the command history.
 * If the specified index is invalid (out of range or negative), it prints an error message.
 * If the index is valid, it copies the corresponding command from history back to the command string.
 *
 * Parameters:
 *  - command_string: The command string that contains the history index (e.g., "!5")
 *  - history: Pointer to the CommandHistory structure containing the history of commands
 *
 * Returns: void
 */
void handleHistoryCommand(char *command_string, CommandHistory *history)
{
  int historyIndex = parseHistoryIndex(command_string);
  if (historyIndex < 0 || historyIndex >= history->count)
  {
    printf("Command not in history.\n");
    command_string[0] = '\0';
    return;
  }
  char *commandFromHistory = getCommandFromHistory(history, historyIndex);
  if (commandFromHistory)
    strcpy(command_string, commandFromHistory);
}
