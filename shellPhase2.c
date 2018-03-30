#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <assert.h>
#include <sys/wait.h>

#define LINEBUFFSIZE 1024
#define TOKENBUFFSIZE 64
#define TOKENDELIM                                                             \
        " \n\t\r\a" // delimiters: space, newline, tab, carriage return, alarm or
                    // beep.
#define SHELLESCAPE "quit"

#define DEBUG // change to 'debug' to enable debug messages

int arg_size = 0;
int hist_count = 0;
int j = 0;
struct history { // structure for history
        int num;
        char *line;
        struct tm tm;
} hist[12];

void nssh_loop();
char *read_cmdline();
char **split_line(char *, int);
int check_redir(char**, int);
int nssh_execute(char **, int);
void add_hist(char *);
void show_hist();
int getch(void);

int main() {
        nssh_loop();
        return 0;
}

void nssh_loop() {
        char *line;
        char **args;
        int status = 1, chg;
        int argsleng = 0;

        do {

                /*while (getch() == 27) { // if the first value is esc ^[
                   getch();              // skip the [
                   switch (getch()) {    // the real value
                   case 65:
                    printf("upp\n");
                    printf("nssh> ");
                    // code for arrow up
                    break;
                   case 66:
                    printf("dowwn\n");
                    printf("nssh> ");
                    // code for arrow down
                    break;
                   }
                   }*/
                printf("nssh> ");
                line = read_cmdline();
                add_hist(line);
                args = split_line(line, arg_size);

                if (args != '\0')
                        status = nssh_execute(args, arg_size);
        } while (status);

        return;
}

char *read_cmdline() {
        /*
         *   function to read input character by character and return
         *   a string(array of char)
         */
        int buffsize = LINEBUFFSIZE;
        int position = 0;

        char *cmd_buff = malloc(sizeof(char) * buffsize);
        int ch, chg;

        if (!cmd_buff) {
                printf("Error allocating the buffer.\n");
                exit(1);
        }

        while (1) {
                ch = getchar();

                if (ch == EOF || ch == '\n') {
                        cmd_buff[position] = '\0';

#ifdef DEBUG
                        printf("cmdbuff: %s\n", cmd_buff);
#endif

                        return cmd_buff;
                } else {
                        cmd_buff[position] = ch;
                }
                position++;

                if (position >= buffsize) {
                        buffsize += LINEBUFFSIZE;
                        cmd_buff = realloc(cmd_buff, buffsize); // if the command is more than
                                                                // 1024 bytes, add another 1024b
                                                                // and realloc
                        if (!cmd_buff) {
                                printf("Error Reallocating the buffer. \n");
                                exit(1);
                        }
                }
        }
        arg_size = position;
}

char **split_line(char *line, int argsleng) {
        int tokbuff = TOKENBUFFSIZE;
        int position = 0;
        char **token_list = malloc(sizeof(char *) * tokbuff);
        char *token;

        if (!token_list) {
                printf("Error allocating Token buffer.\n");
                exit(1);
        }

        token = strtok(line, TOKENDELIM);
        while (token != NULL) {
                token_list[position] = token;
                position++;

                if (position >= tokbuff) {
                        tokbuff += TOKENBUFFSIZE;
                        token_list = realloc(token_list, tokbuff * sizeof(char *));
                        if (!token_list) {
                                printf("Error reallocating token buffer.\n");
                                exit(1);
                        }
                }
                token = strtok(
                        NULL,
                        TOKENDELIM); // to make sure strtok continues search in current string
        }
        token_list[position] = NULL; // null terminate list of tokens
        arg_size = position;
#ifdef DEBUG
        printf("token_list of 0 and 1: %s , %s\n", token_list[0], token_list[1]);
#endif

        return token_list;
}

int nssh_execute(char **argus, int argsleng) {
        int status;
        char** newargs;
        if (strcmp(argus[0], SHELLESCAPE) == 0) { // quit shell
#ifdef DEBUG
                printf("command entered: %s\n", argus[0]);

#endif
                exit(0);
        }
        if (strcmp(argus[0], "cd") == 0) { // change directory for whole shell
                chdir(argus[1]);
                return 1;
        }
        pid_t pid = fork(), wpid;
        if (pid == 0) {
                if (strcmp(argus[0], "showhist") == 0) { // show history of commands
                        show_hist();
                        exit(0);
                }
                /*if((newargs = check_redir(argus)))
                        execvp(newargs[0],newargs); */
              #ifdef DEBUG
                printf("number of args: %d\n", argsleng);
              #endif
                /*  if(check_redir(argus, argsleng) == 1)
                 #ifdef DEBUG
                          printf("in the check_redir IF\n");
                 #endif
                          execvp(argus[0],argus);
                 #ifdef DEBUG
                   printf("outside the check_redir IF\n");
                 #endif*/
                check_redir(argus, argsleng);
                execvp(argus[0], argus);   // exec call to execute commands
        }
        wait(NULL);
        return 1;
}

void add_hist(char *line) {
        /*hist[hist_count % 12] = line;
           hist_count++;*/
        time_t t = time(NULL);

        hist[hist_count % 12].num = j; // j to maintain the order of the commands
        hist[hist_count % 12].line = line;
        hist[hist_count % 12].tm =  *localtime(&t);
        hist_count++; // total number of commands entered in the shell
        j++;
}

void show_hist() {

        for (int i = 0; i < (hist_count); i++) {
                printf("time:%d-%d-%d %d:%d:%d | %d---> %s\n",hist[i].tm.tm_year + 1900, hist[i].tm.tm_mon + 1, hist[i].tm.tm_mday, hist[i].tm.tm_hour, hist[i].tm.tm_min, hist[i].tm.tm_sec, hist[i].num, hist[i].line);
        }
}

int check_redir(char **args, int argsleng){
        char **argscopy;
        int i=0,opredir, inredir;
        /*while(args[i] != NULL) {

                argscopy[i] = args[i];
                if(strcmp(args[i],">")==0) {

                        int found = i;
                        opredir = i;
                        int fd = open(args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0666);
                        dup2(fd, 1);
                        close(fd);
                        for(int j = found; j < tok_size; j++)
                        {
                                argscopy[j] = NULL;
                        }
                        return argscopy;
         #ifdef DEBUG
                        printf("output redir at %d\n", opredir);
         #endif

                }
                /*  if(strcmp(args[i],">")==0) {
                          //      args[i] = NULL;
                          opredir = i;
                          close(1);
                          open(args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0666);
         #ifdef DEBUG
                          printf("output redir at %d\n", opredir);
         #endif
                   }*/
        /*  if(strcmp(args[i],"<")==0) {
                  inredir = i;
                  int fd = open(args[i+1], O_RDONLY);
                  dup2(fd, 0);
                  close(fd);
         #ifdef DEBUG
                  printf("input redir at %d\n", inredir);
         #endif
           }
           i++;
           }*/
        while(args[i] != NULL) {
                if(strcmp(args[i],">")==0) {
                        int fd = open(args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0666);
                        dup2(fd, 1);
                        close(fd);
                        #ifdef DEBUG
                        printf("args inside the check_redir are: %s\n", args[i]);
                        #endif
                        int j = i;
                        while (args[j] != NULL) {
                                args[j] = NULL;
                                j++;
                        }
                        return 1;
                }
                i++;
        }
        return 0;
}




int getch(void) { // implementation of getch function, to extend functionality
                  // of the shell(under development)
        int c = 0;

        struct termios org_opts, new_opts;
        int res = 0;
        //-----  store old settings -----------
        res = tcgetattr(STDIN_FILENO, &org_opts);
        assert(res == 0);
        //---- set new terminal parms --------
        memcpy(&new_opts, &org_opts, sizeof(new_opts));
        new_opts.c_lflag &=
                ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
        c = getchar();
        //------  restore old settings ---------
        res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
        assert(res == 0);
        return (c);
}
