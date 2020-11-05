#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>


#include "defn.h"

//global variables
int globalargc;
char ** globalargv;
char ** gargv;
int shift_count;
int gargc;
int exit_value;

//have to change so output goes to outfd
int built_exit(char** args, int argc, int fd);
int built_envset(char **args, int argc, int fd);
int built_envunset(char **args, int argc, int fd);
int built_cd(char **args, int argc, int fd);
int built_shift(char **args, int argc, int fd);
int built_unshift(char **args, int argc, int fd);
int built_sstat(char **args, int argc, int fd);

int (*built_func[])(char **, int, int) = {built_exit, built_envset, built_envunset, built_cd, built_shift, built_unshift, built_sstat};

// 1 = pass, 0 = not a built name, -1 = built error

int builtin_check(char ** args, int argc, int fd){
  const char * built_names[] = {"exit", "envset", "envunset", "cd", "shift", "unshift", "sstat"};
  int check = 0;
  for(int i = 0; i < 7; i++){
    if(strcmp(args[0], built_names[i]) == 0){
	built_func[i](args,argc, fd);
	check ++;
	break;      
    }
  }
  if(check == 1){
    return 1; //theres a builtin command
  }else {
    return 0; //not a builtin command
  }
}

int built_exit(char ** args, int argc, int fd) {
  if(argc > 1){
    exit(atoi(args[1]));
  }else{
    exit(0);
  }
}

int built_envset(char **args, int argc, int fd){
  if(argc > 2){
    int overwrite = setenv(args[1], args[2], 1); //1 means override if theres a previous variable there
    if(overwrite == 0){
      return 1; //success
    } else{
      printf("Error: Failed to setenv. \n");
      return -1; //error
    }
  }
  else{
    printf("Error: Not enough arguments for envset. \n");
    return -1; //error
  }
}
 
int built_envunset(char **args, int argc, int fd){
  if(argc > 1){
    int value = unsetenv(args[1]);
    if(value == 0){ //unsetenv returns 0 on success
      return 1; //success
    } else{
      printf("Error: Failure to remove env. \n");
      return -1; // error
    }
  }else{
    printf("Error: No value to unset. \n");
    return -1;
  }
}

int built_cd(char **args, int argc, int fd){
  int check = 0;
  if(argc > 1){
    check  = chdir(args[1]);
    if(check  == 0){ //success returns 0
      return 1; 
    } else{
      printf("Error: Failed to change directory. \n");
      return -1; //error returns -1
    }
  }
  else{
    char *direct = getenv("HOME");
    check = chdir(direct);
    if(check == 0){ //success returns 0
      return 1;
    } else{
      printf("Error: Failed to change directory. \n");
      return -1;  //error returns -1
    }
  }
}



int built_shift(char ** args, int argc, int fd){
  int shift_num;
  if(args[1] == 0){
    shift_num = 1;
  }else{
    shift_num = atoi(args[1]);
  }
  if(shift_num  > (globalargc - 1)){
    printf("Error: Shift count greater than arguments given. \n");
    return -1;
  }
  shift_count = shift_count + shift_num;
   //$# decremented by n
  int temp_argc = (globalargc - shift_num);

  int i = 1;
  int count = shift_num;
  while(count < globalargc){
     globalargv[i] = globalargv[shift_num + i];
     count++;
     i++;
   }
  shift_num = 0;
  globalargc = temp_argc;
  return 1;
  
}


//Used gargv as a copy of orignal args so can refil globalargv in unshift
//Used gargc as a copy of orignal length so for unshift can call these two orignal copies to replace the modified ones. 

int built_unshift(char **args, int argc, int fd){
  if(args[1] == 0){
    for(int i = 0; i < gargc; i++){
      globalargv[i] = gargv[i];
    }
    globalargc = (gargc);
    return 1;
  }
  int unshift_num = atoi(args[1]);
  if (unshift_num  > (globalargc - 1)){
    printf("Error: Shift count greater than arguments given. \n");
    return -1;
  }else{   
   //$# incredment by n
   int temp_argc = (globalargc + unshift_num);
   //make sure the correct values are getting replaced and it doesn't fill in values that don't exist
   int i = 1;
   int count = unshift_num;
   shift_count = (shift_count + 1 - unshift_num); 
   int temp = shift_count;
   while(count < gargc){
     globalargv[i] = gargv[temp];
     count++;
     i++;
     temp++;
   }
   //reset shift_count and unshift_num
   shift_count = shift_count - 1;
   unshift_num = 0;
   globalargc = temp_argc;
  }
  return 1;
}


int built_sstat(char **argv, int argc, int fd){
  if(argc > 1){
    int i = 1;
    while(i < argc){
      struct stat *buf;
      struct passwd *user;
      struct group *grp;
      buf = malloc(sizeof(struct stat));
      
      //file name
      char* file = argv[i];
      if(stat(file, buf) == -1){
	printf("Error: No file with that name. \n");
	return -1;
      }

      //user name  
      int uid = buf->st_uid;
      user = getpwuid(uid);
     
      //group name
      int gid = buf->st_gid;
      grp = getgrgid(gid);
      
      //permission list including file type as printed by ls
      mode_t mode = buf->st_mode;
      char p[12];
      strmode(mode, p);

      //number of links
      int links = buf->st_nlink;
      
      //size in bytes
      int size = buf->st_size;
      
      //modification time (localtime and asctime)
      struct tm *time;
      time = localtime(&(buf->st_mtime));
      char *astime = asctime(time);
     
      if((user->pw_name == NULL) || (grp->gr_name == NULL)){	
	dprintf(fd, "%s %d %d %s %d %d %s", argv[i], buf->st_uid, buf->st_gid, p, links, size, astime);
      }else{
      	dprintf(fd, "%s %s %s %s %d %d %s", argv[i], user->pw_name, grp->gr_name, p, links, size, astime);
      }
      free(buf);
      i++;
    }
    return 1;
  }else{
    printf("Error: No file to get stats of \n");
    return -1;
  }
}
