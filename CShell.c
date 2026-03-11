#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>      // open
#include <signal.h>     // signal, SIGCHLD
#include <errno.h>      // errno

#define HISTORY_SIZE 100

/* simple job table for background processes */
typedef struct {
    pid_t pid;
    char cmdline[1024];
    int running;          // 1=running, 0=done
} job_t;

static job_t jobs[HISTORY_SIZE];
static int job_count = 0;

/* tracking for simple undo support */
static char last_cmdline[1024] = "";
static char last_cwd[1024] = "";

// --- Corrected Levenshtein distance ---
int levenshtein(const char *s, const char *t) {
    int ls = strlen(s), lt = strlen(t);
    int matrix[ls+1][lt+1];

    for (int i = 0; i <= ls; i++) matrix[i][0] = i;
    for (int j = 0; j <= lt; j++) matrix[0][j] = j;

    for (int i = 1; i <= ls; i++) {
        for (int j = 1; j <= lt; j++) {
            int cost = (s[i-1] == t[j-1]) ? 0 : 1;

            int del = matrix[i-1][j] + 1;
            int ins = matrix[i][j-1] + 1;
            int sub = matrix[i-1][j-1] + cost;

            int min1 = del < ins ? del : ins;
            int min2 = min1 < sub ? min1 : sub;

            matrix[i][j] = min2;
        }
    }
    return matrix[ls][lt];
}

// --- Suggest closest command from $PATH ---
char* suggest_command(const char *input) {
    char *path_env = getenv("PATH");
    if (!path_env) return NULL;

    char *paths = strdup(path_env);
    char *dir = strtok(paths, ":");
    char best_match[100] = "";
    int best_distance = 1000;

    while (dir) {
        DIR *dp = opendir(dir);
        if (dp) {
            struct dirent *entry;
            while ((entry = readdir(dp))) {
                int dist = levenshtein(input, entry->d_name);
                if (dist < best_distance) {
                    best_distance = dist;
                    strcpy(best_match, entry->d_name);
                }
            }
            closedir(dp);
        }
        dir = strtok(NULL, ":");
    }

    free(paths);

    if (best_distance <= 2 && strcmp(best_match, input) != 0)
        return strdup(best_match);

    return NULL;
}

// --- Completion generator ---
char *completion_generator(const char *text, int state) {
    static DIR *dp = NULL;
    static struct dirent *entry = NULL;
    static char *paths = NULL;
    static char *dir = NULL;
    static int scanning_cwd = 0;

    if (state == 0) {
        if (dp) { closedir(dp); dp = NULL; }
        if (paths) { free(paths); paths = NULL; }

        paths = strdup(getenv("PATH"));
        dir = strtok(paths, ":");
        entry = NULL;
        scanning_cwd = 0;
    }

    // Scan PATH directories
    while (dir) {
        if (!dp) dp = opendir(dir);
        if (dp) {
            while ((entry = readdir(dp))) {
                if (strncmp(entry->d_name, text, strlen(text)) == 0)
                    return strdup(entry->d_name);
            }
            closedir(dp);
            dp = NULL;
        }
        dir = strtok(NULL, ":");
    }

    // Scan current directory
    if (!scanning_cwd) {
        scanning_cwd = 1;
        dp = opendir(".");
        if (!dp) return NULL;

        while ((entry = readdir(dp))) {
            if (strncmp(entry->d_name, text, strlen(text)) == 0)
                return strdup(entry->d_name);
        }
        closedir(dp);
        dp = NULL;
    }

    return NULL;
}

// Hook Readline completion
char **my_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, completion_generator);
}

// --- Split input line into args ---

/* insert spaces around special characters so that strtok can see them */
void preprocess_line(char *line) {
    char buf[4096];
    int j = 0;
    for (int i = 0; line[i]; i++) {
        if (line[i] == '|' || line[i] == '<' || line[i] == '>' || line[i] == '&') {
            buf[j++] = ' ';
            buf[j++] = line[i];
            buf[j++] = ' ';
        } else {
            buf[j++] = line[i];
        }
    }
    buf[j] = '\0';
    strcpy(line, buf);
}

/* split a whitespace‑separated line into arguments */
int parse_line(char *line, char **args) {
    int count = 0;
    char *token = strtok(line, " ");
    while (token) {
        args[count++] = token;
        token = strtok(NULL, " ");
    }
    args[count] = NULL;
    return count;
}

/* handle `<`, `>`, and `>>` tokens; modifies args array in place */
void setup_redirection(char **args) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i+1]) {
                int fd = open(args[i+1], O_RDONLY);
                if (fd < 0) perror("open");
                else {
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                int j = i;
                while (args[j+2]) { args[j] = args[j+2]; j++; }
                args[j] = NULL;
                args[j+1] = NULL;
                i--;
            }
        } else if (strcmp(args[i], ">") == 0) {
            if (args[i+1]) {
                int fd = open(args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0666);
                if (fd < 0) perror("open");
                else {
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                int j = i;
                while (args[j+2]) { args[j] = args[j+2]; j++; }
                args[j] = NULL;
                args[j+1] = NULL;
                i--;
            }
        } else if (strcmp(args[i], ">>") == 0) {
            if (args[i+1]) {
                int fd = open(args[i+1], O_WRONLY|O_CREAT|O_APPEND, 0666);
                if (fd < 0) perror("open");
                else {
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                int j = i;
                while (args[j+2]) { args[j] = args[j+2]; j++; }
                args[j] = NULL;
                args[j+1] = NULL;
                i--;
            }
        }
    }
}

/* job management routines */
void add_job(pid_t pid, const char *cmdline) {
    if (job_count < HISTORY_SIZE) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmdline, cmdline,
                sizeof(jobs[job_count].cmdline) - 1);
        jobs[job_count].cmdline[sizeof(jobs[job_count].cmdline)-1] = '\0';
        jobs[job_count].running = 1;
        job_count++;
    }
}

void update_jobs(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                jobs[i].running = 0;
                break;
            }
        }
    }
}

void print_jobs(void) {
    update_jobs();
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s %s\n", i+1, jobs[i].pid,
               jobs[i].running ? "Running" : "Done",
               jobs[i].cmdline);
    }
}

void bring_job_foreground(int idx) {
    if (idx < 0 || idx >= job_count) {
        fprintf(stderr, "fg: no such job\n");
        return;
    }
    if (jobs[idx].running) {
        int status;
        if (waitpid(jobs[idx].pid, &status, 0) == -1)
            perror("waitpid");
        jobs[idx].running = 0;
    }
}

void background_job(int idx) {
    if (idx < 0 || idx >= job_count) {
        fprintf(stderr, "bg: no such job\n");
        return;
    }
    if (!jobs[idx].running) {
        if (kill(jobs[idx].pid, SIGCONT) == -1)
            perror("kill");
        else
            jobs[idx].running = 1;
    }
}

void sigchld_handler(int sig) {
    (void)sig;
    update_jobs();
}

/* create a pipeline of ncmds, return pid of last child */
pid_t execute_pipeline(char *cmds[][50], int ncmds, int background) {
    int in_fd = STDIN_FILENO;
    int pipes[2];
    pid_t pid;
    pid_t last_pid = -1;

    for (int i = 0; i < ncmds; i++) {
        if (i < ncmds - 1) {
            if (pipe(pipes) < 0) {
                perror("pipe");
                return -1;
            }
        }

        pid = fork();
        if (pid == 0) {
            /* child */
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < ncmds - 1) {
                dup2(pipes[1], STDOUT_FILENO);
                close(pipes[0]);
                close(pipes[1]);
            }
            setup_redirection(cmds[i]);
            execvp(cmds[i][0], cmds[i]);
            perror("exec failed");
            exit(1);
        } else if (pid < 0) {
            perror("fork failed");
            return -1;
        } else {
            if (in_fd != STDIN_FILENO)
                close(in_fd);
            if (i < ncmds - 1) {
                close(pipes[1]);
                in_fd = pipes[0];
            }
            last_pid = pid;
        }
    }

    if (!background) {
        int status;
        for (int i = 0; i < ncmds; i++)
            wait(&status);
        update_jobs();
    }
    return last_pid;
}

int main() {
    char *line;
    char *args[50];

    rl_attempted_completion_function = my_completion;
    /* ensure we notice when background children exit */
    signal(SIGCHLD, sigchld_handler);

    while (1) {
        // Prompt with cwd
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        char prompt[1050];
        snprintf(prompt, sizeof(prompt), "%s%s ", cwd, (geteuid()==0)?"#":"$");

        line = readline(prompt);
        if (!line) break;
        if (line[0] != '\0') add_history(line);

        /* keep copy for history/undo logic */
        char original_line[1024];
        strncpy(original_line, line, sizeof(original_line)-1);
        original_line[sizeof(original_line)-1] = '\0';

        int count = parse_line(line, args);
        if (count == 0) { free(line); continue; }

        /* redo previous command? */
        if (strcmp(args[0], "redo") == 0 || strcmp(args[0], "!!") == 0) {
            if (last_cmdline[0] == '\0') {
                printf("nothing to redo\n");
                free(line);
                continue;
            }
            /* echo for user; typical shells print the command */
            printf("%s\n", last_cmdline);
            /* replace line contents and reparse */
            strncpy(line, last_cmdline, strlen(last_cmdline)+1);
            strncpy(original_line, last_cmdline, sizeof(original_line)-1);
            original_line[sizeof(original_line)-1] = '\0';
            count = parse_line(line, args);
            if (count == 0) { free(line); continue; }
        }

        /* check for undo builtins first */
        if (strcmp(args[0], "fuck") == 0 || strcmp(args[0], "shit") == 0) {
            if (last_cmdline[0] == '\0') {
                printf("nothing to undo\n");
            } else if (strncmp(last_cmdline, "cd", 2) == 0) {
                if (last_cwd[0]) {
                    if (chdir(last_cwd) != 0)
                        perror("cd undo");
                }
            } else if (strncmp(last_cmdline, "touch ", 6) == 0) {
                char *fn = last_cmdline + 6;
                if (unlink(fn) == -1)
                    perror("undo touch");
            } else if (strncmp(last_cmdline, "mkdir ", 6) == 0) {
                char *dn = last_cmdline + 6;
                if (rmdir(dn) == -1)
                    perror("undo mkdir");
            } else if (strstr(last_cmdline, ">") != NULL) {
                /* simplistic: remove file redirected-to if it exists */
                char *gt = strrchr(last_cmdline, '>');
                if (gt) {
                    gt++;
                    while (*gt == ' ')
                        gt++;
                    char *end = strchr(gt, ' ');
                    if (end) *end = '\0';
                    if (unlink(gt) == -1)
                        perror("undo redirect");
                }
            } else {
                printf("can't undo '%s'\n", last_cmdline);
            }
            free(line);
            continue;
        }

        /* save state for potential undo of this command */
        if (getcwd(last_cwd, sizeof(last_cwd)) == NULL)
            last_cwd[0] = '\0';
        strncpy(last_cmdline, original_line, sizeof(last_cmdline)-1);
        last_cmdline[sizeof(last_cmdline)-1] = '\0';

        /* add spacing around special characters so we can split pipes/redirects */
        preprocess_line(line);

        /* split into commands separated by '|'; maximum of 10 segments */
        char *cmds[10][50];
        int ncmds = 0, argidx = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(args[i], "|") == 0) {
                cmds[ncmds][argidx] = NULL;
                ncmds++;
                argidx = 0;
            } else {
                cmds[ncmds][argidx++] = args[i];
            }
        }
        cmds[ncmds][argidx] = NULL;
        ncmds++;

        /* background job indicator ('&' at end) */
        int background = 0;
        if (ncmds > 0) {
            int last = 0;
            while (cmds[ncmds-1][last])
                last++;
            if (last > 0 && strcmp(cmds[ncmds-1][last-1], "&") == 0) {
                background = 1;
                cmds[ncmds-1][last-1] = NULL;
            }
        }

        /* built‑in commands (only when a single non-background command) */
        if (ncmds == 1 && !background) {
            char *cmd = cmds[0][0];
            if (cmd) {
                if (strcmp(cmd, "exit") == 0) {
                    free(line);
                    break;
                }
                if (strcmp(cmd, "cd") == 0) {
                    if (cmds[0][1]) {
                        if (chdir(cmds[0][1]) != 0)
                            perror("cd failed");
                    } else {
                        char *home = getenv("HOME");
                        if (home) chdir(home);
                    }
                    free(line);
                    continue;
                }
                if (strcmp(cmd, "history") == 0) {
                    HIST_ENTRY **hist_list = history_list();
                    if (hist_list) {
                        for (int i = 0; hist_list[i]; i++)
                            printf("%d  %s\n", i+1, hist_list[i]->line);
                    }
                    free(line);
                    continue;
                }
                if (strcmp(cmd, "jobs") == 0) {
                    print_jobs();
                    free(line);
                    continue;
                }
                if (strcmp(cmd, "fg") == 0) {
                    if (cmds[0][1])
                        bring_job_foreground(atoi(cmds[0][1]) - 1);
                    free(line);
                    continue;
                }
                if (strcmp(cmd, "bg") == 0) {
                    if (cmds[0][1])
                        background_job(atoi(cmds[0][1]) - 1);
                    free(line);
                    continue;
                }
            }
        }

        /* autocorrect only for the very first command */
        char *suggestion = NULL;
        if (cmds[0][0])
            suggestion = suggest_command(cmds[0][0]);
        if (suggestion) {
            printf("Command '%s' not found. Did you mean '%s'? [y/N]: ",
                   cmds[0][0], suggestion);
            fflush(stdout);

            int resp = getchar();
            while (getchar() != '\n');

            if (resp == 'y' || resp == 'Y') {
                cmds[0][0] = suggestion;
            } else {
                free(suggestion);
                free(line);
                continue;
            }
        }

        /* run either a single program or a pipeline */
        pid_t last_pid = execute_pipeline(cmds, ncmds, background);
        if (background && last_pid > 0) {
            add_job(last_pid, line);
            printf("[%d] %d\n", job_count, last_pid);
        }

        free(line);
    }

    printf("Bye!\n");
    return 0;
}
