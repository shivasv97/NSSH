#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <assert.h>
#include <sys/wait.h>

#include"stopwatch.c"
#include"timer.c"

#define LINEBUFFSIZE 1024
#define TOKENBUFFSIZE 64
#define ALIASIZE 20
#define TOKENDELIM                                                             \
        " \n\t\r\a" // delimiters: space, newline, tab, carriage return, alarm or
                    // beep.
#define SHELLESCAPE "quit"
#define READ_END 0
#define WRITE_END 1

#define NODEBUG // change to 'DEBUG' to enable debug messages


struct nlist { /* table entry: */
        char *name; /* defined name */
        char *defn; /* replacement text */
};
struct nlist aliases[ALIASIZE];
int alias_count = 0;


struct history { // structure for history
        int num;
        char *line;
        struct tm tm;
} hist[12];
int arg_size = 0;
int hist_count = 0;
int j = 0;

struct dll                    //definition of the structure double linked list
{
        char s[200];                    //character array
        int index;
        struct dll*prev,*next;    //pointers to next and previous nodes
};



void nssh_loop();
char *read_cmdline();
char **split_line(char *, int);
int check_redir(char**, int);
int check_piping(char**, int);
void pied_piper(char***, int);
int check_alias(char*);
char ***pipe_commands(char**, int, int);
int nssh_execute(char **, int);
void add_hist(char *);
void show_hist();
void create_alias(char*, char*);
void lsorto();
void speakcmd(char**);
void speakop(char**);
int getch(void);

void editor_initialize();
void editcommands(void);                    //editor function prototypes
void addline(struct dll *temp);
void inp(void);
void printlist(void);
void closer(void);
void edit(void);
void addnode(char t[],struct dll *q);
void delnode(struct dll *p);
void clealist(void);
void editnode(struct dll *p);
void save(void);

struct dll *head;                        //header node declaration
char file[20];

FILE *fp=NULL;                            //file pointer declaration

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
        int pipe_num;
        char** newargs;
        char*** pipe_comm;
        speakcmd(argus);
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
        if((strcmp(argus[0],"alias"))==0) {
                create_alias(argus[1],argus[2]);
        }
        if((strcmp(argus[0],"lsorto"))==0) {
                lsorto();
        }
        if((strcmp(argus[0],"spkme"))==0) {
                speakop(argus);
        }

        pid_t pid = fork(), wpid;
        if (pid == 0) {
                int i=-1;
                i=check_alias(argus[0]);
                if(i > -1) {
                        char* temp;
                        char* temptok;
                        //argus[0] = aliases[i].name;
                        temp = aliases[i].name;
                        temptok = strtok(temp," ");
                        argus[0] = temptok;
                        temptok = strtok(NULL, " ");
                        argus[1] = temptok;

                }
                
        	if((strcmp(argus[0],"stopwatch"))==0) {
                stopwatch();
        	}if((strcmp(argus[0],"timer"))==0) {
                timer();
        	}
                if (strcmp(argus[0], "showhist") == 0) { // show history of commands
                        show_hist();
                        exit(0);
                }
                if (strcmp(argus[0], "eddy") == 0) { // open editor
                        editor_initialize();
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
                #ifdef DEBUG
                printf("args after the check redir call: %s, %s\n", argus[0], argus[1]);
                #endif
                if (pipe_num = check_piping(argus, argsleng)>1) {
                        pipe_comm = pipe_commands(argus, argsleng, pipe_num);
                        pied_piper(pipe_comm, pipe_num);
                }
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
                        int fd = open(args[i+1], O_WRONLY|O_CREAT, 0666);
                        dup2(fd, 1);
                        close(fd);
                        /*close(1);
                           open(args[i+1], O_WRONLY|O_CREAT, 0666);*/
                        #ifdef DEBUG
                        printf("args inside the check_redir > are: %s\n", args[i]);
                        #endif
                        int j = i;
                        while (args[j] != NULL) {
                                //while(j < argsleng) {
                                args[j] = NULL;
                                j++;
                        }
                        return 1;
                }

                if(strcmp(args[i],"<")==0) {
                        int fd = open(args[i+1], O_RDONLY, 0555);
                        dup2(fd, 0);
                        close(fd);
                        #ifdef DEBUG
                        printf("args inside the check_redir < are: %s\n", args[i]);
                        #endif
                        /*  int j = i;
                           while (args[j] != NULL) {
                         #ifdef DEBUG
                                  printf("new args are: %s\n", args[j]);
                         #endif
                                  args[j] = NULL;
                                  j++;
                           }*/
                        return 1;
                }
                i++;
        }
        return 0;
}


int check_piping(char** args, int argsleng){
        int pipe_count=0;
        for (int i =0; i < argsleng; i++) {
                if (strcmp(args[i],"|")==0) {
                        pipe_count++;
                }
        }
        #ifdef DEBUG
        printf("number of pipes: %d\n", pipe_count);
        #endif
        return pipe_count+1;
}


char*** pipe_commands(char** args, int argsleng, int pipe_num){
        int i = 0, k =0, j =0;
        char*** pipe_comm = (char***) malloc((pipe_num) * sizeof(char**));
        pipe_comm[k] = (char**) malloc(64 * sizeof(char*));
        for(i=0; args[i]!=NULL; i++) {

                if(strcmp(args[i], "|") == 0) {
                        k++;
                        j=0;
                        pipe_comm[k] = (char**) malloc(64*sizeof(char*));

                }
                else{
                        pipe_comm[k][j] = (char*) malloc(100 * sizeof(char));
                        strcpy(pipe_comm[k][j],args[i]);
                        #ifdef DEBUG

                        printf("in the pipe_comm: %s and arg: %s\n", pipe_comm[k][j], args[i]);

                        #endif
                        j++;
                }

        }
        #ifdef nonDEBUG
        for(int i=0; i <= pipe_num; i++) {
                printf("pipe commands: %s\n", *(pipe_comm+i));
        }
        #endif
        return pipe_comm;
}

void pied_piper(char*** pipe_comm, int pipe_num){
        if(pipe_num<0) {
                return;
        }
        int p[2];
        pipe(p);
        int forkid = fork();

        if(forkid == 0) {
                close(1);
                dup(p[WRITE_END]);
                close(p[READ_END]);
                pied_piper(pipe_comm,pipe_num-1);
                #ifdef debug
                printf("commands executing in child fork: %s\n", pipe_comm[pipe_num-1][0]);
                #endif
                close(p[WRITE_END]);
                execvp(pipe_comm[pipe_num-1][0],pipe_comm[pipe_num-1]);
        }
        else if(forkid > 0) {
                if(pipe_num>0) {
                        close(0);
                        dup(p[READ_END]);
                }
                close(p[WRITE_END]);
                wait(NULL);
                close(p[READ_END]);
                #ifdef debug
                printf("commands executing in parent fork: %s\n", pipe_comm[pipe_num][0]);
                #endif
                execvp(pipe_comm[pipe_num][0],pipe_comm[pipe_num]);

        }
        return;
}

int check_alias(char* args){
        for(int i = 0; i < alias_count; i++)
        {
                if(!strcmp(args,aliases[i].defn))
                {
                        return i;
                }
        }
        return -1;
}

void editor_initialize(){
        char c;

        head=(struct dll*)malloc(sizeof(struct dll)); //header node memory allocation
        (head->next)=(head->prev)=NULL;  //initialization
        (head->index)=0;

        while(1)  //infinite while loop for editing multiple number of tiles
        {

                system("clear"); //clearing the screen

                //Displaying editor options
                printf("\nThis Editor provides the following options \n");
                printf("R :opens a file and reads it into a buffer\n   If file doesnot exist creates a new file for editing\n");
                printf("E :edit the currently open file\n");
                printf("X :closes the current file and allows you to open another file\n");
                printf("Q :quit discarding any unsaved changes\n");

                c=getch(); //taking user input
                switch(c) //testing with switch
                {
                case 'r':
                case 'R':
                        inp();
                        break;
                case 'e':
                case 'E':
                        edit();
                        break;
                case 'x':
                case 'X':
                        closer();
                        break;
                case 'q':
                case 'Q':
                        system("clear");
                        exit(1);
                        break;
                }
        }

}

void addnode(char t[],struct dll *q)        //function to add a new node after a node q
{
        struct dll*p=(struct dll*)malloc(sizeof(struct dll));
        struct dll *temp=q->next;
        strcpy(p->s,t);
        p->prev=q;
        p->next=q->next;

        if((q->next)!=NULL) //adding the node to the list by manipulating pointers accordingly
        {
                ((q->next)->prev)=p;
                while(temp!=NULL)
                {
                        (temp->index)++; //incrementing the index of the later nodes

                        temp=temp->next;
                }
        }
        q->next=p;
        p->index = q->index + 1;            //setting the index of the new node
}

void delnode(struct dll *p)                    //function to delete a node
{
        struct dll *temp=p->next;
        ((p->prev->next))=p->next;
        if(p->next!=NULL)
        {
                ((p->next)->prev)=p->prev;
                while(temp!=NULL)
                {
                        (temp->index)--; //decrementing the index of the later nodes

                        temp=temp->next;
                }
        }
        free(p);                //freeing ht memory of the deleted node
}


void clearlist(void)                        //function to clear the list
{
        while(head->next!=NULL)
                delnode(head->next);    //deleting all nodes except head
}



void editnode(struct dll *p)                    //function to edit a line
{
        printf("\nThe original line is :\n%s",p->s);
        printf("\nEnter the new line :\n");
        gets(p->s);                    //taking the new line input
        printf("\nLine edited\n");
}


void printlist(void)            //function to print all the lines stored in the buffer
{
        struct dll *temp=head;
        system("clear");
        while(temp->next!=NULL)
        {
                temp=temp->next;
                printf("%d %s\n",temp->index,temp->s); //printing the lines on the screen
        }
}



void closer(void)                //function to close the file orened for editing
{
        if(fp==NULL)
                return;
        fclose(fp);
        fp=NULL;
        clearlist();                //the list is also cleared at this point
}



void inp(void)
{
        struct dll *buff=head;                //temporaty variable
        char c;
        char buf[200];                    //array to store input line

        if(fp!=NULL)                    //checking for files opened earlier
        {
                printf("\nThere is another file open it will be closed\ndo you want to continue ?(Y/N):");
                c=getch();
                if(c=='n'||c=='N')
                        return;
                else
                        closer();
        }

        fflush(stdin);
        printf("\nEnter the file name you want to open :");
        scanf("%s",file);
        getchar();
        fflush(stdin);
        clearlist();

        fp=fopen(file,"r");                //opening the specified file
        if(fp==NULL)                 //checking if the file previously exists
        {
                printf("\nThe file does not exist do you want to create one?(Y/N) :");
                c=getchar();
                getchar();
                if(c=='N'||c=='n')
                        return;
                else
                {
                        fp=fopen(file,"w"); //creating new file
                        edit();
                        return;
                }
        }

        if(feof(fp))
                return;

        while((fgets(buf,201,fp))!=NULL)        //taking input from file
        {
                addnode(buf,buff);
                buff=buff->next;
        }
        edit();                    //calling the edit function
}



void edit(void)                            //the edit function
{
        struct dll *temp=head->next;  //pionter used to mark the current position during traversing
        char c,d;

        system("clear");                //clearing the screen

        if(fp==NULL)                //checking for files previously open
        {
                printf("\nNo file is open\n");
                return;
        }

        printf("\nThe file contents will be displayed along with the line number\npress any key\n");
        getch();
        system("clear");
        printlist();                    //printing the entire buffered list
        if(temp!=NULL)
                printf("You are at line number %d",temp->index); //printing the line number of control
        else
                temp=head;

        editcommands();            //prints the list of commands available

        while(1)                //infinite loop for multiple command usage
        {
                c=getch();

                switch(c)        //switch -->condition checkig
                {
                case 'c':
                case 'C':

                        editnode(temp); //edit the current line pointed to by temp
                        break;

                case 'p':
                case 'P':     //move to the previous line
                        if(temp==head)
                        {
                                printf("\nFile empty"); //message displayed if list is empty
                                break;
                        }
                        if(temp->prev!=head)
                        {
                                temp=temp->prev;
                                printf("\nYou are at line number %d",temp->index);
                        }
                        else //message display if already at first line

                                printf("\nalready at first line");
                        break;

                case 'n':
                case 'N':     //move to the next line
                        if(temp->next!=NULL)
                        {
                                temp=temp->next;
                                printf("\nYou are at line number %d",temp->index);
                        }
                        else if(temp==head)
                                printf("\nFile empty"); //message printed if list is empty
                        else
                                printf("\nalready at last line"); //message printed if already at last line
                        break;

                case 'a':
                case 'A': //adding a new line after node ponted by temp
                        addline(temp); //addline function takes input and creates a new node via addnode function
                        temp=temp->next;
                        printlist();
                        printf("\nYou are at line number %d",temp->index);
                        break;

                case 'h':
                case 'H':     //HELP command displays the list of available commmands
                        system("clear");
                        editcommands(); //notice that there is no break as after help the entire list is printed
                        system("clear");

                case 'v':
                case 'V':     //printing the entire list via printlist function
                        printlist();
                        printf("\nYou are at line number %d",temp->index);
                        break;

                case 'D':
                case 'd':     //deleting a line pointed to by temp
                        if(temp==head) //checking if list is empty
                        {
                                printf("\nFile empty\n");
                                break;
                        }
                        temp=temp->prev;
                        delnode(temp->next); //deleting the node
                        printf("\nLine deleted\n");
                        printlist(); //printing the list
                        if(temp->index)
                                printf("\nYou are currently at line number %d",temp->index);
                        if(((temp->prev)==NULL)&&(temp->next)!=NULL)
                                temp=temp->next;
                        else if((temp==head)&&((temp->next)==NULL))
                                printf("\nFile empty"); //printing message if list is empty
                        break;

                case 'X':
                case 'x':     //exit the editor to main menu

                        printf("\nDo you want to save the file before exiting?(y/n) :");

                        d=getch(); //warning for saving before exit
                        if(d=='y'||d=='Y')
                                save();
                        closer();
                        return;
                        break;

                case 's':
                case 'S':     //saving and exitting
                        save();
                        closer();
                        return;
                        break;

                }

        }

}


void addline(struct dll *temp)                    //adding a new line via input from user
{
        char buff[200];
        printf("\nenter the new line :\n");
        gets(buff);                    //taking the new line
        addnode(buff,temp);                //ceatng the new node
}


void save(void)                            //function to save the file
{
        struct dll *temp=head->next;
        fclose(fp);
        fp=fopen(file,"w");                //opening the file in write mode

        while(temp!=NULL)
        {
                fprintf(fp,"%s\n",temp->s);    //writing the linked list contents to file
                temp=temp->next;
        }

}


void editcommands(void)                        //function to print the list of editer commands
{
        printf("\nEditor commands\n");
        printf("The edit menu provides the following options \n");
        printf("C :edit the current line\n");
        printf("P :move one line up\n");
        printf("N :move one line down\n");
        printf("D :delete the current line\n");
        printf("V :display the contents of the buffer\n");
        printf("A :add a line after the line at which you are navigating\n");
        printf("S :save changes and exit to main menu\n");
        printf("X :exit discarding any changes\n");
        printf("H :show the list of commands\n");
        getch();
}

/************************************************************************/
/************************************************************************/
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
/************************************************************************/
/************************************************************************/
void create_alias(char* tok1, char* tok2){

        //char temp[20]; strtok_r(tok,"=", &)
        //temp = argv[1];

        //char str[] = *tok;
        printf("tok1: %s\n",tok1);
        printf("tok2: %s\n",tok2);
        char *tt;
        char *rest = tok1;
        int i = 0;
        char* strArray[2];
        while ((tt = strtok_r(rest, "=", &rest)))
        {
                strArray[i] = malloc(strlen(tt) + 1);
                strcpy(strArray[i],tt);
                printf("-->>%s\n",strArray[i]);
                i++;
        }
        char *name;
        name = (char*)malloc(sizeof(char)*100);
        char *alias_name;
        alias_name = (char*)malloc(sizeof(char)*100);
        strcpy(alias_name,strArray[0]);
        strcpy(name,strArray[1]);
        printf("%s\n",name);
        printf("%s\n",alias_name);
        if(tok2 != NULL) {
                char sp[2];
                sp[0] = ' ';
                sp[1] = '\0';
                strcat(name,sp);
                //printf("%s\n",name);
                strcat(name,tok2);
                printf("-|->%s\n",name);
        }



        /*
           char *name;
           char *alias_name;
           int i = 0;
           char *n = strtok(tok,"=");
           alias_name = n;
           char *o = strtok(NULL,"=");

           name=o; */
        int found = -1;
        for(int i = 0; i < alias_count; i++)
        {
                if(!(strcmp(aliases[i].name,name)))
                {
                        found = i;
                        break;
                }
        }
        if(found == -1)
        {
                struct nlist a;
                a.name = name;
                a.defn = alias_name;
                aliases[alias_count] = a;
                alias_count++;
                printf("Aliases count : %d\n", alias_count);
        }

        else
        {
                printf("THe previous alias %s will be changed to %s\n",aliases[found].defn,alias_name );
                aliases[found].defn = alias_name;
        }

        for(int i = 0; i < alias_count; i++)
        {
                printf("%s : %s\n",aliases[i].name,aliases[i].defn );
        }


}

void lsorto(){
	system("ls | python lsorto.py");
}

void speakcmd(char** args){
	char buf[1024];
	char a[64] ;
	strcpy(a, args[0]);
	snprintf(buf, sizeof(buf), "espeak %s",a);
	system(buf);
}
void speakop(char** args){
	char buf[1024];
	char a[64] ;
	strcpy(a, args[1]);
	strcat(a," ");
	strcat(a, args[2]);
	snprintf(buf, sizeof(buf), "%s | espeak",a);
	system(buf);
}

/*******************************/
int tokenize_pipe()
{
        char str[] = "ls -l|gedit one.txt|gcc hello.c";
        char *token;
        char *rest = str;
        int i = 0;
        char* strArray[50];

        while ((token = strtok_r(rest, "|", &rest)))
        {
                strArray[i] = malloc(strlen(token) + 1);
                strcpy(strArray[i], token);
                printf("%s\n", strArray[i]);
                i++;
        }
        return(0);
}
