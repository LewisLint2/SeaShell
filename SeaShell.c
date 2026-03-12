#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <glob.h>
#include <dirent.h>
#include <errno.h>
#include "linenoise.h"
#include <signal.h>
#define HISTORY_FILE ".seashellhist"
#define MAX_ARGS 512
#define MAX_CMDS 64
#define MAX_LINE 8192

char *last_command = NULL;
void sigint_handler(int signo) {
    // Just print a newline and redisplay prompt
    // write(STDOUT_FILENO, "\n", 1);
    (void)write(STDOUT_FILENO, "\n", 1);
    linenoiseClearScreen();  // optional: clear current input line
}
struct command {
    char *argv[MAX_ARGS];
    char *input;
    char *output;
    int append;
    int background;
};

// Expand environment variables like $HOME
void expand_vars(char **arg) {
    if ((*arg)[0] == '$') {
        char *env = getenv((*arg) + 1);
        if (env) {
            free(*arg);
            *arg = strdup(env);
        }
    }
}

// Expand wildcards *, ?
// Expand wildcards *, ? (works in directories too)
void expand_glob(struct command *cmd) {
    glob_t g;
    char *newargv[MAX_ARGS];
    int argc = 0;

    for (int i = 0; cmd->argv[i]; i++) {
        // Only try to glob if pattern contains * or ?
        if (strchr(cmd->argv[i], '*') || strchr(cmd->argv[i], '?')) {
            // Use flags to handle directories and ~ expansion
            if (glob(cmd->argv[i], GLOB_NOCHECK | GLOB_MARK | GLOB_TILDE | GLOB_BRACE, NULL, &g) == 0) {
                for (size_t j = 0; j < g.gl_pathc; j++)
                    newargv[argc++] = strdup(g.gl_pathv[j]);
                globfree(&g);
            }
        } else {
            newargv[argc++] = strdup(cmd->argv[i]);
        }
    }
    newargv[argc] = NULL;

    // Free old argv
    for (int i = 0; cmd->argv[i]; i++) free(cmd->argv[i]);
    for (int i = 0; i <= argc; i++)
        cmd->argv[i] = newargv[i];
}

// Parse a command string into struct command
void parse_command(char *cmd, struct command *c) {
    int argc = 0;
    c->input = NULL;
    c->output = NULL;
    c->append = 0;
    c->background = 0;

    char *token = strtok(cmd, " \t\n");
    while (token) {
        if (strcmp(token, "<") == 0) {
            char *next = strtok(NULL, " \t\n");
            if (next) c->input = strdup(next);
            else fprintf(stderr, "syntax error: expected file after <\n");
        } else if (strcmp(token, ">") == 0) {
            char *next = strtok(NULL, " \t\n");
            if (next) { c->output = strdup(next); c->append = 0; }
            else fprintf(stderr, "syntax error: expected file after >\n");
        } else if (strcmp(token, ">>") == 0) {
            char *next = strtok(NULL, " \t\n");
            if (next) { c->output = strdup(next); c->append = 1; }
            else fprintf(stderr, "syntax error: expected file after >>\n");
        } else if (strcmp(token, "&") == 0) {
            c->background = 1;
        } else {
            c->argv[argc++] = strdup(token);
            expand_vars(&c->argv[argc-1]);
        }
        token = strtok(NULL, " \t\n");
    }
    c->argv[argc] = NULL;
    expand_glob(c);
}

// Split a line into pipeline commands
int split_pipeline(char *line, char *cmds[]) {
    int n = 0;
    cmds[n] = strtok(line, "|");
    while (cmds[n] && n < MAX_CMDS - 1) {
        n++;
        cmds[n] = strtok(NULL, "|");
    }
    return n;
}

// Execute a pipeline
void execute_pipeline(struct command cmds[], int n) {
    int pipes[MAX_CMDS][2];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) < 0) { perror("pipe"); exit(1); }
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) { // child
            signal(SIGINT, SIG_DFL);  // reset SIGINT to default in child
            if (i > 0) dup2(pipes[i-1][0], STDIN_FILENO);
            if (i < n-1) dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < n-1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (cmds[i].input) {
                int fd = open(cmds[i].input, O_RDONLY);
                if (fd < 0) { perror(cmds[i].input); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmds[i].output) {
                int fd;
                if (cmds[i].append)
                    fd = open(cmds[i].output, O_WRONLY|O_CREAT|O_APPEND, 0644);
                else
                    fd = open(cmds[i].output, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if (fd < 0) { perror(cmds[i].output); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(cmds[i].argv[0], cmds[i].argv);
            perror("exec");
            exit(1);
        }
    }

    for (int i = 0; i < n-1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (!cmds[n-1].background) {
        for (int i = 0; i < n; i++) wait(NULL);
    }
}

// Tab completion
void completion(const char *buf, linenoiseCompletions *lc) {
    DIR *d = opendir(".");
    struct dirent *dir;
    if(d){
        while((dir=readdir(d))){
            if(strncmp(dir->d_name, buf, strlen(buf))==0)
                linenoiseAddCompletion(lc, dir->d_name);
        }
        closedir(d);
    }

    char *path_env = getenv("PATH");
    if (!path_env) return;
    char *path = strdup(path_env);
    char *p = strtok(path, ":");
    while(p){
        DIR *dp = opendir(p);
        if(dp){
            while((dir=readdir(dp))){
                if(strncmp(dir->d_name, buf, strlen(buf))==0)
                    linenoiseAddCompletion(lc, dir->d_name);
            }
            closedir(dp);
        }
        p = strtok(NULL, ":");
    }
    free(path);
}

// Prompt: user@host:cwd$
// Prompt: user@host:cwd$ (normal) or user@host:cwd# (root)
char *get_prompt() {
    static char prompt[MAX_LINE];
    char cwd[512], hostname[256];

    (void)getcwd(cwd, sizeof(cwd));
    gethostname(hostname, sizeof(hostname));

    char *user = getenv("USER");
    if (!user) user = "user";

    char end_char = (getuid() == 0) ? '#' : '$'; // root = #, normal = $

    snprintf(prompt, sizeof(prompt), "%s@%s:%s %c ", user, hostname, cwd, end_char);
    return prompt;
}
// Free command memory
void free_commands(struct command cmds[], int n) {
    for (int i=0;i<n;i++){
        for(int j=0; cmds[i].argv[j]; j++) free(cmds[i].argv[j]);
        if(cmds[i].input) free(cmds[i].input);
        if(cmds[i].output) free(cmds[i].output);
    }
}

int main() {
    signal(SIGINT, SIG_IGN);
    linenoiseSetCompletionCallback(completion);
    linenoiseSetMultiLine(1);  // enable multi-line editing for long commands
    linenoiseHistoryLoad(HISTORY_FILE);

    while(1){
        char *line = linenoise(get_prompt());
        if(!line) break;

        if(strcmp(line,"!!")==0){
            if(!last_command){
                printf("no history\n");
                free(line);
                continue;
            }
            free(line);
            line = strdup(last_command);
            printf("%s\n",line);
        }

        if(strlen(line)==0){ free(line); continue; }

        linenoiseHistoryAdd(line);

        free(last_command);
        last_command = strdup(line);

        if(strcmp(line,"exit")==0){ free(line); break; }

        if(strncmp(line,"cd ",3)==0){
            if(chdir(line+3)<0) perror("cd");
            free(line);
            continue;
        }

        char *cmd_strings[MAX_CMDS];
        struct command cmds[MAX_CMDS];

        int n = split_pipeline(line,cmd_strings);
        for(int i=0;i<n;i++)
            parse_command(cmd_strings[i],&cmds[i]);

        execute_pipeline(cmds,n);
        free_commands(cmds,n);

        free(line);
    }

    free(last_command);
    // Before exiting main()
    linenoiseHistorySave(HISTORY_FILE);
    return 0;
}