#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "print current directory"},
  {cmd_cd, "cd", "change directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

int cmd_pwd(unused struct tokens *tokens){
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  if(cwd == NULL)
    return 1;
  else
    fprintf(stdout,"%s\n", cwd);
  return 0;
}

int cmd_cd(unused struct tokens *tokens){
  char* dir;
  if(tokens == NULL)
    return 1;
  else{
    dir = tokens_get_token(tokens, 1);
    if(dir != NULL)
      chdir(dir);
  }
  return 0;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  //redirection enable
  int redirectoken = -1;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      //fprintf(stdout, "This shell doesn't know how to run programs.\n"
      char pathHolder[4096];
      char* headPtr = NULL, *tailPtr = NULL;
      headPtr = getenv("PATH");
      while((tailPtr = strchr(headPtr, ':')) != NULL){
        //make full path binary
        int pathLen = tailPtr - headPtr;
        strncpy(pathHolder, headPtr, pathLen);
        pathHolder[pathLen] = '/';

        int tokenlen = tokens_get_length(tokens);
        if(tokenlen > 0)
          strncpy(pathHolder + pathLen + 1, tokens_get_token(tokens, 0), 4096 - pathLen - 1);

        //make array argv
        char** argvlist = malloc(sizeof(char*) * tokenlen +1);
        for(int i = 0; i < tokenlen; i++){
          argvlist[i] = tokens_get_token(tokens, i);
          if(strcmp(argvlist[i], ">") == 0 || strcmp(argvlist[i], "<") == 0){
            redirectoken = i;
          }
        }
        argvlist[0] = pathHolder;
        argvlist[tokenlen] = NULL;

        //fork and execute if file is executeable
        if(access(pathHolder,F_OK) == 0 && access(pathHolder, X_OK) == 0){
          printf("%s exist and execuable\n", pathHolder);
          // int infd = 3, outfd = 4;
          // dup2(0, infd);
          // dup2(1, outfd);
          pid_t pidfork;
          if((pidfork = fork()) == -1) printf("fork error\n");

          if(pidfork == 0){
            printf("fork and execv %s\n", pathHolder);
            //thread redirection
            if(redirectoken != -1){
              int fdirect;
              switch (argvlist[redirectoken][0]) {
                case '<':
                  argvlist[redirectoken] = NULL;
                  fdirect = open(argvlist[redirectoken+1], O_CREAT|O_TRUNC|O_RDONLY, 0644);
                  dup2(fdirect, 0);
                  execv(pathHolder, argvlist);
                case '>':
                  argvlist[redirectoken] = NULL;
                  fdirect = open(argvlist[redirectoken+1], O_CREAT|O_TRUNC|O_WRONLY, 0644);
                  dup2(fdirect, 1);
                  execv(pathHolder, argvlist);
              }
            }
            else{
              execv(pathHolder, argvlist);
            }
          printf("execv return false value %s\n", strerror(errno));
          exit(0);
          }
          else {
            waitpid(pidfork, NULL, 0);
            // dup2(infd, 0);
            // dup2(outfd, 1);
          }
          break;
        }
        else{
          // printf("command %s %s\n",pathHolder, strerror(errno));
        }
        headPtr = tailPtr + 1;
      }

    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
