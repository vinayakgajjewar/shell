#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *PROMPT = "> ";

#define LSH_RL_BUFSIZE 1024

char *read_line(void) {
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;
    if (!buffer) {
        fprintf(stderr, "Error allocating buffer space\n");
        exit(EXIT_FAILURE);
    }
    while (1) {
        c = getchar();

        /*
         * If we hit EOF, replace it with a null character and return. We'll hit
         * EOF if the user hits Ctrl-D or if we reach the end of the text file
         * that we're reading commands from.
         */
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = (char) c;
        }
        position++;

        /*
         * If we've exceeded the buffer, reallocate space.
         */
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            char *new_buffer = realloc(buffer, bufsize);
            if (!new_buffer) {
                fprintf(stderr, "Error reallocating buffer space\n");
                exit(EXIT_FAILURE);
            } else {
                buffer = new_buffer;
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

/*
 * We're just going to use whitespace to separate arguments. Quoting or
 * backslash escaping will not be supported.
 */
char **split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;
    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            char **new_tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!new_tokens) {
                fprintf(stderr, "Allocation error\n");
                exit(EXIT_FAILURE);
            } else {
                tokens = new_tokens;
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int launch(char **args) {
    pid_t pid;
    __attribute__((unused)) pid_t wpid;
    int status;
    pid = fork();
    if (pid == 0) {

        /*
         * We are in the child process.
         */
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {

        /*
         * There was an error forking.
         */
        perror("lsh");
    } else {

        /*
         * We are in the parent process.
         */
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

/*
 * Need to forward-declare this to break the circular dependency between
 * lsh_help and builtin_func.
 */
int lsh_cd(char **args);

/*
 * Need to forward-declare this to break the circular dependency between
 * lsh_help and builtin_func.
 */
int lsh_help();

/*
 * Need to forward-declare this to break the circular dependency between
 * lsh_help and builtin_func.
 */
int lsh_exit();

char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&lsh_cd, &lsh_help, &lsh_exit};

int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help() {
    int i;
    printf("Welcome to my shell\n");
    printf("Type program name and arguments and hit ENTER\n");
    printf("The following commands are built in:\n");
    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("\t%s\n", builtin_str[i]);
    }
    printf("Use the \"man\" command for information about other programs\n");
    return 1;
}

/*
 * Signal the command loop to terminate.
 */
int lsh_exit() {
    return 0;
}

int execute(char **args) {

    /*
     * Handle the case where the user enters an empty string or nothing but
     * whitespace.
     */
    if (args[0] == NULL) {
        return 1;
    }
    for (int i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return launch(args);
}

void loop(void) {
    char *line;
    char **args;
    int status;
    do {
        printf("%s", PROMPT);
        line = read_line();
        args = split_line(line);
        status = execute(args);
        free(line);
        free(args);
    } while (status);
}

int main(void) {
    loop();
    return EXIT_SUCCESS;
}