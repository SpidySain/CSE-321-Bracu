// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

// Global values
#define cmd_len 1024
#define max_arg 100
#define history_size 50

// Global variable
char history[history_size][cmd_len]; // define history
int hcount=0;  // track of history count

// Functions
void exec_process(char *command);
void exec_cmd_and(char *command);
int exec_cmd(char *command);
void piping(char *command);
void redirections(char *arg[]);
void history_adding(char *command);
void history_print();
void signal_handlar(int sig);
void quotes(char *string);

// Actual Code:
int main(){
    signal(SIGINT, signal_handlar);
    char command[cmd_len];
    while (1){
       printf("sh> ");
       fflush(stdout);     
       
       if (fgets(command, cmd_len, stdin)==NULL){  // handle ctrl + d
         if (feof(stdin)){
           printf("\n");
           break;                                  
         }
       }
   
       command[strcspn(command, "\n")]=0;
       if (strlen(command) > 0){
         exec_process(command);
       }
    }
    return 0;
}

// Handel Quotes in the printing line
void quotes(char *string){
  int len=strlen(string);
  if (len>=2 && ((string[0]=='"' && string[len-1]=='"') || (string[0]=='\'' && string[len-1]=='\''))){
    memmove(string, string+1, len-2);
    string[len-2]='\0';
  }
}

//Handle CTRL + C
void signal_handlar(int sig){
     printf("\nsh> ");
     fflush(stdout);
}

// Process Command
void exec_process(char *command){
     history_adding(command);
     
     if (strcmp(command, "exit")==0){
       exit(0);
     }
     else if (strcmp(command, "history")==0){
       history_print();
       return;
     }
     char *cmd = strtok(command, ";");
     while (cmd != NULL){
        exec_cmd_and(cmd);
        cmd=strtok(NULL, ";");
     }
} 

//Execution
void exec_cmd_and(char *command){
     char *cmd[max_arg];
     int i=0;
     char *token= strtok(command, "&&");
     while (token!=NULL){
       cmd[i++]=token;
       token= strtok(NULL, "&&");
     }
     
     cmd[i]=NULL;
     
     for (int j=0; j<i; j++){
       if (exec_cmd(cmd[j])!=0){
         break;
       }
     }
}
// Execution and Piping
int exec_cmd(char *command){
     if (strchr(command, '|')){
       piping(command);
       return 0;
     }
     
     char *arg[max_arg];
     int i = 0;
     char *token = strtok(command, " ");
     while (token){
       arg[i++]=token;
       token= strtok(NULL, " ");
     }
     
     arg[i]=NULL;
     for (int j=0; arg[i]!=NULL;j++){
       quotes(arg[j]);
     }
     
     if (strcmp(arg[0], "cd")==0){
       if (arg[1]==NULL){
         fprintf(stderr, "cd: missing argument\n");
       }
       else{
         if (chdir(arg[1]) !=0){
           perror("cd failed");
         }
       }
     return 0;
     }
     
     pid_t pid=fork();
     if (pid<0){
       perror("Fork Failed");
       return 1;
     }
     else if (pid==0){
       redirections(arg);
       execvp(arg[0], arg);
       perror("Exec Failed");
       exit(EXIT_FAILURE);
     }
     else{
       int st;
       waitpid(pid, &st,0);
       return WEXITSTATUS(st);
     }   
}

//Piping
void piping(char *command){
   int pipefd[2];
   pid_t pid1, pid2;
   char *cmd1=strtok(command, "|");
   char *cmd2=strtok(NULL, "|");
   
   if (pipe(pipefd)==-1){
     perror("Piping Failed");
     return;
   }
   pid1=fork();
   pid2=fork();
   
   if (pid1==0){
     dup2(pipefd[1], STDOUT_FILENO);
     close(pipefd[0]);
     close(pipefd[1]);
     exec_cmd(cmd1);
     exit(EXIT_FAILURE);
   }
   
   if (pid2==0){
     dup2(pipefd[0], STDIN_FILENO);
     close(pipefd[1]);
     close(pipefd[0]);
     exec_cmd(cmd2);
     exit(EXIT_FAILURE);
   }
   

   
   close(pipefd[0]);
   close(pipefd[1]);
   wait(NULL);
   wait(NULL);
}

// Redirections
void redirections(char *arg[]){
   int i=0;
   for (i; arg[i]!=NULL; i++){
   
     if (strcmp(arg[i], "<") == 0){
       int fd = open(arg[i+1], O_RDONLY);
       dup2(fd, STDIN_FILENO);
       close(fd);
       arg[i]=NULL;
     }
     
     else if(strcmp(arg[i], ">")==0){
       int fd = open(arg[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
       dup2(fd, STDOUT_FILENO);
       close(fd);
       arg[i]=NULL;
     }
     
     else if(strcmp(arg[i], ">>")==0){
       int fd = open(arg[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
       dup2(fd, STDOUT_FILENO);
       close(fd);
       arg[i]=NULL;
     }
   }
}

// Add commands to the history array
void history_adding(char *command){
  if (hcount < history_size){
    strcpy(history[hcount++], command);
  }
  else{
    for (int i=1; i<history_size; i++){
      strcpy(history[i-1],history[i]);
    }
    strcpy(history[history_size-1],command);
  }
}

// Print History
void history_print(){
  for (int i=0; i<hcount; i++){
    printf("%d: %s\n", i+1, history[i]);
  }
}

