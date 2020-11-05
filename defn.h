#define WAIT 1
#define EXPAND 1
#define NOWAIT 0
#define NOEXPAND 0

int processline(char *line, int fd, int outfd, int flags, int expand_flag);
int expand(char *orig, char *new, int newsize);
int builtin_check(char ** args, int argc, int fd);
void strmode(mode_t mode, char *p);
//global variables

int globalargc;
char ** globalargv;
char ** gargv;
int shift_count;
int gargc;
int exit_value;
