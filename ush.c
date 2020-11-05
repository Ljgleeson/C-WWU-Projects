//Liam Gleeson, Assignment4
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>

#include "defn.h"


/* Constants */ 

#define LINELEN 1024

/* Prototypes */

int processline(char *line, int fd, int outfd, int flags, int expand_flag);
void handler(int status);
char ** arg_parse (char *line, int *argcptr);
void pipeline(char *line, int count);

int globalargc;
char ** globalargv;
int gargc;
char ** gargv;
int shift_count;
int exit_value;


//make argc and argv global varibles (not these but literal argc)


int
main (int mainargc, char ** mainargv)
{
  globalargc = mainargc;
  globalargv = mainargv;
  gargc = mainargc;
  //hard copy another version of mainargv
  gargv = (char**) malloc((mainargc + 1) * sizeof(char *));
  gargv[mainargc] = NULL;

  for(int i = 0; i < mainargc; i++){
     gargv[i] = mainargv[i];
  }

    char   buffer [LINELEN];
    int    len;
    FILE *fptr;
    
    
     if(mainargc>1){
      fptr = fopen(mainargv[1], "r");
      if(fptr == NULL){
	printf("Error: No file with that name \n");
	exit(127);
      }
    }
     //  printf("this is mainargv at 3: %s \n", mainargv[3]);
    
	while (1) {
	  
	  if(mainargc>1){
	    stdin = fptr;
	  }
	    else{
	    fprintf (stderr, "%% ");
	   }
	  
	  if (fgets (buffer, LINELEN, stdin) != buffer)
	    break;
	  
	  
	  /* Get rid of \n at end of buffer. */
	  len = strlen(buffer);
	  if (buffer[len-1] == '\n')
	    buffer[len-1] = 0;

	  int i = 0;
	  while(i < len){
	    if(buffer[i] == '$' && buffer[i+1] == '#'){
	      i += 2;
	    }else if(buffer[i] == '#'){
	      while(i < len){
		buffer[i] = 0;
		i++;
	      }
	    }
	    else{
	      i++;
	    }
	  }
	  
	/* Run it ... */

	  
	  processline (buffer, 0, 1, WAIT, EXPAND);

	} // while end bracket


    if (!feof(stdin))
        perror ("read");

    return 0;
    /* Also known as exit (0); */
    
}

//returns 0 if successful and 1 if unsuccessful

int processline(char *line, int infd, int outfd, int flags, int expand_flag)
{
  
    pid_t  cpid;
    int    status;    
    int argc;
    char new_line [LINELEN];
    char ** args;
    int fd[2];

    //1 = EXPAND = do expansions.
    if(expand_flag == 1){
      int ex =  expand(line, new_line, LINELEN);
      if(ex != 1){ //expand was successful
	return - 1;
      }
      if(pipe(fd) < 0){
	exit(1);
      }
    }else{
      int len = strlen(line);
      for(int i = 0; i < len; i++){
	new_line[i] = line[i];
      }
    }
    
    //pipe check:
    char *pipe_check;
    char *pipe_temp;     
    int check = 1;
    int old_fd;
    int k = 0;
    int q = 1;
    while((pipe_check = strchr(new_line, '|')) != NULL){	
	pipe_check[k] = '\0';
	
	if(check == 1){ //if first one read from stdin, write to fd[1]
	  printf("we are in first one \n");
	  processline(new_line, 0, fd[1], NOWAIT, NOEXPAND);
	  close(fd[1]);
	  old_fd = fd[0];
	  check = 0;
	  
	}
	else{ //middle check, read from fd[0], write to fd[1]
	  if(pipe(fd) < 0){
	    exit(1);
	  }
	  processline(pipe_temp, old_fd, fd[1], NOWAIT, NOEXPAND);
	  close(fd[1]);
	  close(old_fd);
	  old_fd = fd[0];
	  
	}
	pipe_temp = &pipe_check[q];
	if ((pipe_check = strchr(new_line, '|')) == NULL) { //last one, read from fd[0], write to stdout
	  processline(pipe_temp, old_fd, 1, NOWAIT, NOEXPAND);
	  close(old_fd);
	  break;
	}
	k++;
	q++;	
		
    }    
		
    args = arg_parse(new_line, &argc);

    if(argc == 0){
      return - 1;
    }
    
    int built = builtin_check(args, argc, outfd);
    
    //use 0 if built-in is successful
    if(built == 1){ //builtin successful
      exit_value = 0;
      free(args);
    }else if(built == -1){ //built in failure
      exit_value = 1;
      return -1;
    }else{ //run program regularly as no builtin commands found
      /* Start a new process to do the job. */
      cpid = fork();
      if (cpid < 0) {
	/* Fork wasn't successful */
	perror ("fork");
	return -1; //return -1;
      }    
      /* Check for who we are! */
      if (cpid == 0) {
	/* We are the child! */
	
	if(outfd != 1){
	  int fd = dup2(outfd, 1);  //dup2 moves outfd to 1
	  if(fd == -1){
	    perror("dup2");
	  }
	}
	
	if(infd != 0){
	  int fd = dup2(infd, 0);
	  if(fd == -1){
	    perror("dup2");
	  }
	}

	  
	//swithced to execvp;uses 2 parameters: char file and the const. argv
	execvp (args[0], args);
	
	  /* execlp reurned, wasn't successful */
	  perror ("exec");
	  fclose(stdin);  // avoid a linux stdio bug
	  exit (127);
	}  
	else{
	  //free pointer if its in parent shell
	  free(args);
	  args=NULL; //this could be doing problesmf or me
	}
	/* Have the parent wait for child to complete */
	//might have to change wait for waitpid

	if(flags & WAIT){
	  if (wait (&status) < 0) {
	    /* Wait wasn't successful */
	    perror ("wait");
	    return -1;

	  }
	  else{	  
	    //wait was successful: we ware waiting on a child
	    handler(status);
	  }
	}
	else{
	  free(args);
	  return(cpid);
	}
    }
    free(args);
    return 0;
}

//created helped function that has finds the correct value for exit or error or quit
void handler(int status){
  
  int sig_num = WTERMSIG(status);
  //get signal number and check to make sure its not sigit// sigint value is 2
  if(sig_num != 2){
    //if command called exit, take the passed passed into that function.
    if(WIFEXITED(status)){
      exit_value = WEXITSTATUS(status);
    }	    
    //if command was killed by a signal, use number 128 plus signal numer
    if(WIFSIGNALED(status)){
      exit_value = (128 + sig_num);

      //determine if core dumped, if true print core dumped
      if(WCOREDUMP(status)){
	printf(" (core dump) \n");
      }    
    }
  }
  else{
    if(WIFEXITED(status)){
      exit_value = WEXITSTATUS(status);
    }
    printf(" Sigint faliure \n");
  }

}


char ** arg_parse(char *line, int *argcptr){  
  int argc = 0;
  int i = 0;
  int length = strlen(line);
  
  //count the args: 
  while(i < length ){
    if(line[i] == ' '){
      i++;
    }
    else{
      argc++;
      while(line[i] != ' '){
	if(i == length){
	  break;
	}
	else if(line[i] == '\"'){
	  i++;
	  while(line[i] != '\"'){
	    i++;
	    if(i == length){
	      printf("unmatched quotes \n");
	      return NULL;
	    }
	  }
	i++;
	}
	else{
	  i++;
	  if(i == length){
	   break;
	  }
	}
      }
    }
  }
    
  //malloc the array of correct size:
  char ** arg = (char**) malloc((argc + 1) * sizeof(char *));
  arg[argc] = NULL;
  if(arg==NULL){
    printf("error with memory allocation");
    return NULL;
  }

  
  //Assign the pointers and set zero values:
  int j = 0;
  int ctr = 0;
  while(j < length){
    if(line[j] == ' '){
      j++;
    }
    else{
    arg[ctr] = &line[j];
    ctr++;
    while(line[j] != ' '){
      if(j == length){
	break;
      }
      else if(line[j] == '\"'){
	j++;
	while(line[j] != '\"'){
	  j++;
	  if(j == length){
	    return NULL;
	  }
	}
      j++;
      }
      else{
	j++;
	if(j == length){
	  break;
	}
      }
    }
     line[j] = 0;
     j++;
    }
  }

  //remove quotes
  // I'm aware this doesnt work for all cases but I ran out of time and had to submit so I apologize
  
  for(int x = 0; x < argc; x++){
    for(int y = 0; y < strlen(arg[x]); y++){
      if(arg[x][y] == '\"'){
       	while(arg[x][y+1] != '\"'){
	  arg[x][y] = arg[x][y+1];
	  y++;
	}
	// we know next value is " so jump over it and keep going
	while(arg[x][y+2] != ' '){
	  arg[x][y] = arg[x][y+2];
	  y++;
	}	
      }
      else{
	y++;
      } 
    }
  }
  
  //use argcptr to save argc to the correct place
  *argcptr = argc;
  return arg;
  
}
