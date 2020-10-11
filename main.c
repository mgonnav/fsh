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
char history[MAX_LINE_WIDTH] = "";
int redirection, redirection_pos;
int pipe_pos;
bool should_wait;

bool HistoryCommand(char*);
bool StrEq(char*, char*);
void ParseLineToArgs(char*, char**);
void OpenRedirectionDescriptorIfSpecified(char**);
void CopyPipedCommand(char** args1, char** args2);

int main() {
  bool should_run = true;
  char* args[MAX_ARGS];
  fdin = dup(STDIN_FILENO);
  fdout = dup(STDOUT_FILENO);

  while (should_run) {
    printf("fsh>");
    fflush(stdout);

    char line[MAX_LINE_WIDTH];
    scanf("%[^\n]s", line);
    while ((getchar()) != '\n')
      ;

    if (HistoryCommand(line)) {
      if (*history) {
        strcpy(line, history);
      } else {
        printf("No commands in history.\n");
        continue;
        continue;
      }
    }

    redirection = -1;
    pipe_pos = -1;
    should_wait = true;
    ParseLineToArgs(line, args);

    if (StrEq(args[0], "exit")) break;

    OpenRedirectionDescriptorIfSpecified(args);

    char* args2[MAX_ARGS];
    CopyPipedCommand(args, args2);

    int fd[2];
    pid_t pid = fork();
    if (pid < 0) {
      printf("ERROR: Couldn't fork.\n");
      exit(0);
    } else if (pid == 0) {
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
      }
    } else {
      if (should_wait) wait(NULL);
    }

    dup2(fdin, STDIN_FILENO);
    dup2(fdout, STDOUT_FILENO);
  }

  return 0;
}

bool HistoryCommand(char* line) { return StrEq(line, "!!"); }

bool StrEq(char* s1, char* s2) { return strcmp(s1, s2) == 0; }

void ParseLineToArgs(char* line, char** args) {
  int j = 0;
  args[j] = strtok(line, " ");
  redirection = -1;
  pipe_pos = -1;
  while (args[j] != NULL) {
    args[++j] = strtok(NULL, " ");
    if (StrEq(args[j - 1], "<")) {
      args[j - 1] = NULL;
      fdin = dup(STDIN_FILENO);
      redirection = 0;
      redirection_pos = j - 1;
    } else if (StrEq(args[j - 1], ">")) {
      args[j - 1] = NULL;
      fdout = dup(STDOUT_FILENO);
      redirection = 1;
      redirection_pos = j - 1;
    } else if (StrEq(args[j - 1], "|")) {
      args[j - 1] = NULL;
      pipe_pos = j - 1;
    }
  }

  if (StrEq(args[j - 1], "&")) {
    should_wait = false;
    args[j - 1] = NULL;
  }
}

void OpenRedirectionDescriptorIfSpecified(char** args) {
  int file_desc;
  if (redirection == 0) {
    file_desc = open(args[redirection_pos + 1], O_RDONLY);
    dup2(file_desc, STDIN_FILENO);
  } else if (redirection == 1) {
    args[redirection_pos] = NULL;
    file_desc = open(args[redirection_pos + 1], O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRWXU | S_IRWXG);
    dup2(file_desc, STDOUT_FILENO);
  }
}

void CopyPipedCommand(char** args1, char** args2) {
  if (pipe_pos != -1) {
    int i;
    for (i = pipe_pos + 1; args1[i] != NULL; ++i)
      args2[i - pipe_pos - 1] = args1[i];
    args2[i - pipe_pos - 1] = NULL;
  }
}
