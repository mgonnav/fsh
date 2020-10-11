#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE_WIDTH 80
#define MAX_TOKEN_WIDTH 25
#define MAX_ARGS MAX_LINE_WIDTH / 2 + 1

#define READ_END 0
#define WRITE_END 1

int fdin, fdout;

int main() {
  bool should_run = true;
  char *args[MAX_ARGS];
  char history[MAX_LINE_WIDTH] = "";

  while (should_run) {
    printf("fsh>");
    fflush(stdout);

    char line[MAX_LINE_WIDTH];
    scanf("%[^\n]s", line);
    if (strcmp(line, "!!") == 0) {
      if (!*history) {
        printf("No commands in history.\n");
        while ((getchar()) != '\n')
          ;
        continue;
      } else {
        strcpy(line, history);
      }
    }

    while ((getchar()) != '\n')
      ;

    strcpy(history, line);

    int j = 0;
    args[j] = strtok(line, " ");
    int redirection_pos;
    int redirection = 0;
    int pipe_pos = -1;
    while (args[j] != NULL) {
      args[++j] = strtok(NULL, " ");
      if (strcmp(args[j - 1], "<") == 0) {
        fdin = dup(STDIN_FILENO);
        redirection = 1;
        redirection_pos = j - 1;
      } else if (strcmp(args[j - 1], ">") == 0) {
        fdout = dup(STDOUT_FILENO);
        redirection = 2;
        redirection_pos = j - 1;
      } else if (strcmp(args[j - 1], "|") == 0) {
        args[j - 1] = NULL;
        pipe_pos = j - 1;
      }
    }

    if (strcmp(args[0], "exit") == 0) break;

    int file_desc;
    int saved_desc;
    if (redirection == 1) {
      saved_desc = dup(STDIN_FILENO);
      file_desc = open(args[redirection_pos + 1], O_RDONLY);
      dup2(file_desc, redirection - 1);
    } else if (redirection == 2) {
      saved_desc = dup(STDOUT_FILENO);
      args[redirection_pos] = NULL;
      file_desc = open(args[redirection_pos + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG);
      dup2(file_desc, redirection - 1);
    }

    bool should_wait = true;
    if (strcmp(args[j - 1], "&") == 0) {
      should_wait = false;
      args[j - 1] = NULL;
    }

    char *args2[MAX_ARGS];
    if (pipe_pos != -1)
    {
      int i;
      for (i = pipe_pos+1; args[i] != NULL; ++i)
        args2[i-pipe_pos-1] = args[i];
      args2[i-pipe_pos-1] = NULL;
    }

    int fd[2];
    pid_t pid = fork();
    if (pid < 0) {
      printf("ERROR: Couldn't fork.\n");
      exit(0);
    } else if (pid == 0) {
      if (redirection == 1) {
        args[redirection_pos] = NULL;
        read(STDIN_FILENO, args[redirection_pos], 200);
        args[redirection_pos + 1] = NULL;
      }

      if (pipe_pos != -1) {
        pipe(fd);

        if (fork() == 0) {
          fdout = dup(STDOUT_FILENO);
          dup2(fd[WRITE_END], STDOUT_FILENO);
          close(fd[READ_END]);
          execvp(args[0], args);
        } else {
          wait(NULL);
          fdin = dup(STDIN_FILENO);
          dup2(fd[READ_END], STDIN_FILENO);
          close(fd[WRITE_END]);

          execvp(args2[0], args2);
        }
      } else {
        execvp(args[0], args);
        exit(0);
      }
    } else {
      if (should_wait) wait(NULL);
    }

    dup2(fdin, STDIN_FILENO);
    dup2(fdout, STDOUT_FILENO);
  }

  return 0;
}