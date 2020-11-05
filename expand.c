#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "defn.h"

//orig = input string
//new = ptr to fixed size array similar to buffer in main.
//newsize = number of characters in the new array


//global variables
int globalargc;
char ** globalargv;
char ** gargv;
int shift_count;
int gargc;
int exit_value;


int expand (char *orig, char *new, int newsize){

  int holder = 0;
  int count = 0;
  int len = strlen(orig);
  int i = 0;
  int z = 0;
  int num = 0;
  char new_arr[newsize];
  
  //used ascii values for $ and brackets 
  while(i < len ){
    if(z >= newsize){
      printf("Error: expansion is too long.\n");
      return 0;
    }
    //${...} check
    else{
      if ((orig[i] == 36) && (orig[i+1] == 123)){
	i = i + 2;
	holder = i;
	while(orig[i] != 125){
	  count++;
	  i++;
	  if(i == len){
	    printf("Error: No closing bracket. \n");
	    return 0;
	  }
	}
	i++;
	for(int j = 0; j < count; j++){;
	  new_arr[j] = orig[holder];
	  holder++;
	}
	int arr_len = strlen(getenv(new_arr));
	char *envi = getenv(new_arr);
	int y = 0;
	while(y < arr_len){
	  if(z >= newsize){
	    printf("Error: expansion is too long. \n");
	    return 0;
	  }
	  new[z] = envi[y];
	  y++;
	  z++;
	}
	//reset count to 0 so can call again if multiple ${...} used
    	count = 0;
      }
      
      //$$ check
      else if ((orig[i] == 36) && (orig[i+1] == 36)){
	i = i + 2;
	pid_t pid = getpid();
	char str [100];
	snprintf(str, 100,"%d", pid);
	int pid_len = strlen(str);
	int w = 0;
	while(w < pid_len){
	  if(z >= newsize){
	    printf("Error: expansion is too long. \n");
	    return 0;
	  }
	  new[z] = str[w];
	  z++;
	  w++;
	}
      }

      //$#
      else if ((orig[i] == 36) && (orig[i+1] == 35)) {
      	i = i + 2;
	char str[100];
	snprintf(str,100,"%d",(globalargc-1));
	int k = 0;
	int argc_len = strlen(str);
	while(k < argc_len){
	  new[z] = str[k];
	  z++;
	  k++;
	}
      }

      //$n
      else if ((orig[i] == 36) && (isdigit(orig[i+1]) != 0)) {
	num = (orig[i+1] - 48);	
	if(num >= (globalargc - 1)){
	  orig[i] = 0;
	  orig[i+1] = 0;
	  i = i + 2;
	}else{
	  i = i + 2;
	  int argv_len = strlen(globalargv[num+1]);
	  char str[100];
	  snprintf(str,100,"%s", globalargv[num+1]);
	  int k = 0;
	  while(k < argv_len){
	    new[z] = str[k];
	    z++;
	    k++;
	  }
	}
      }
      
      // \* check 
      else if((orig[i] == 92) && (orig[i+1] == 42)){
	i++;
	new[z] = orig[i];
	i++;
	z++;
      }
      
      // * with trailing values check
      else if((orig[i] == '*') && (orig[i+1] != 0 && orig[i+1] != ' ')) {
	struct dirent *de;
	DIR *dr = opendir(".");
	if(dr == NULL){
	  printf("Error: no directory to open. \n");
	  return 0;
	}
	int x = i + 1;
	int len = 0;
	//get proper length to fill array
	while(orig[x] != 0){
	  x++;
	  len++;
	}
	i++;
	int y = 0;
	char str[len];
	//fill array with characters after *
	while(orig[i] != 0) {
	  str[y] = orig[i];
	  i++;
	  y++;
	}
	//set last value to 0;
	str[len] = 0;
	//while loop that checks to find values that end with str specified after *
	while((de = readdir(dr)) != NULL){
	  int length = strlen(de->d_name);
	  int x = 0;
	  int check = 0;
	  while(x < len){
	    if(str[x] == de->d_name[length-len+x]){
	      check++;
	    }
	    x++;
	  }
	  int j = 0;
	  if(check == len){
	    while(j < length){
	      new[z] = de->d_name[j];
	      j++;
	      z++;
	    }
	    new[z] = ' ';
	    z++;
	  }
	}
	closedir(dr);  
      }

      
     //* without trailing values check
      else if ( (orig[i] == ' ') && ((orig[i+1] == '*') && (orig[i+2] == ' ' || orig[i+2] == 0)) ) {
	new[z] = ' ';
	z++;
	struct dirent *de;
	DIR *dr = opendir(".");
	if(dr == NULL){
	  printf("Error: no directory to open. \n");
	  return 0;
	}
	while((de = readdir(dr)) != NULL){
	  int length = strlen(de->d_name);
	  int x = 0;
	  if(de->d_name[0] != '.'){
	    while(x < length){
	      new[z] = de->d_name[x];
	      x++;
	      z++;
	    }
	    new[z] = ' ';
	    z++;
	  }
	}
	closedir(dr);
	i = i + 2;
      }
      // $? check:
      //NOTE: Doesn't work with echo commands, sometimes adds a value after it that was from previous echo statement.
      else if ((orig[i] == '$') && (orig[i+1] == '?')){
      	i = i + 2; 
	char str[100];
	sprintf(str,"%d",(exit_value));
	int k = 0;
	int argc_len = strlen(str);
	while(k < argc_len){
	  new[z] = str[k];
	  z++;
	  k++;
	}
	z++;
	
     }
      //$(...) check
      else if ((orig[i] == '$') && (orig[i+1] == '(') ){
	i = i + 2;
	int open_parn = 1;
	int close_parn = 0;
	int cmd_length = 0;
	int a = i;

        //found $(, find matching ) but make sure (.count == ).count
	while(open_parn != close_parn){	  
	   if(a >= newsize){
	     fprintf(stderr, "Error: unmatched \n");
	     return -1;
	  }else if(orig[a] == '('){
	      open_parn ++;
	      cmd_length++;
	      a++;
	  }else if (orig[a] == ')'){
	      close_parn ++;
	      cmd_length ++;
	      a++;
	  }else{
	    cmd_length++;
	    a++;
	  }
	}

	char pipe_cmd[cmd_length];
	a = 0;
	//fill pipe_cmd with values within (  )
	while(a < cmd_length){
	  pipe_cmd[a] = orig[i];
	  a++;
	  i++;
	}	
	//temporarily store a end-of-string over the ).
	pipe_cmd[cmd_length - 1] = '\0';

	//create the pipe
	int fd[2];
	if(pipe(fd) < 0){
	  printf("pipe");
	  return -1;
	}

	//call processline on new cmd.
	int cpid = processline(pipe_cmd, 0, fd[1], NOWAIT, EXPAND);


	//put back the ')' into the command string
	pipe_cmd[cmd_length - 1] = ')';
	
	//printf("This is pipe_cmd After processline call %s \n", pipe_cmd);
	close(fd[1]); //close write

	//-1 = error
	if(cpid != -1){
	  while(1){
	    int rv = read(fd[0], &new[z], newsize);

	    if(rv == 0 || z >= newsize){
	      break;
	    }
	    z = z + rv;
	  }
	  close(fd[0]); //close read
	  int status;
	  if(waitpid(cpid, &status, 0) < 0){
	    perror("waitpid");
	    return -1;
	  }else{
	      
	    int sig_num = WTERMSIG(status);
	    //get signal number and check to make sure its not sigit
	    if(sig_num != SIGINT){
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
	      //sigint failure.
	      if(WIFEXITED(status)){
		exit_value = WEXITSTATUS(status);
	      }	  
	      printf(" Sigint faliure \n");
	    }
	  }
	}

	int v = 0;
	int new_length = strlen(new);
	while(v < new_length){
	  //make all \n to 
	  if (new[v] == '\n'){
	    new[v] = ' ';
	  }
	  v++;
	}
	
      }
      
      else{
	if(z >= newsize){
	  printf("Error: expansion is too long. \n");
	  return 0;
	}
	new[z] = orig[i];
	i++;
	z++;
      }
    }
   
  }

  new[z] = 0;
  

      
  //1 = pass, 0 = fail. 
  if(z < newsize){
    return 1;
  } else {
    printf("Error: Size greater than buffer.");
    return 0;
  }


  
}
