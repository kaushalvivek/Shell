#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>

#define rep(i, a, b) for(i = a; i < b; i++)
#define rev(i, a, b) for(i = a; i > b; i--)
#define TOK_BUFFER 64
#define COMM_BUFFER 64
#define TOK_DELIM " \t\n\a\r"
#define COMM_DELIM ";"
char* root;
int shell_cd (char **args);
int shell_exit (char **args);
int shell_quit(char **args);
int shell_pwd (char **args);
int shell_echo (char **args);
int shell_pinfo (char **args);
int shell_ls (char **args);
int shell_setenv (char **args);
int shell_unsetenv (char **args);

char *builtin_arr[] = {"cd","exit","pwd","echo","pinfo", "ls","setenv","unsetenv","quit"};
int (*builtin_functions[])(char**) = {&shell_cd,&shell_exit,&shell_pwd,&shell_echo,&shell_pinfo,&shell_ls,&shell_setenv,&shell_unsetenv,&shell_quit};


void sigintHandler(int sig_num) {
	signal(SIGINT,sigintHandler);
	fflush(stdout);
}


void pipehandler(char **args){
	// File descriptors
	int filedes[2];
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int err = -1,end = 0, i = 0, j = 0,k = 0,l = 0;
	
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	
	while (args[j] != NULL && end != 1){
		k = 0;
	
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				end = 1;
				k++;
				break;
			}
			k++;
		}
		command[k] = NULL;
		j++;		
		if (i % 2 != 0){
			pipe(filedes); 
		}else{
			pipe(filedes2); 
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){

			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}

			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){
					dup2(filedes[0],STDIN_FILENO);
				}else{ 
					dup2(filedes2[0],STDIN_FILENO);
				}
			}else{ 
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ 
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}		
		}
				
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(filedes[0]);
			}else{					
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(filedes2[0]);
				close(filedes[1]);
			}else{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
}


int redirect(char **args, int num)
{
	char in[100],out[100],app[100];
	int r = 0,pid,flag1=0,flag2=0,flag3=0;
	args[num] = NULL;

	pid = fork();
	if (pid==0)
	{	
		for (int i=0;i<num;i++)
		{
			if (strcmp(args[i],"<")==0)
			{
				args[i] = NULL;
				strcpy(in,args[i+1]);
				flag1 = 1;
			}
			
			if (args[i]!=NULL)
			{
				if (strcmp(args[i],">")==0)
				{
					args[i] = NULL;
					strcpy(out,args[i+1]);
					flag2 = 1;
				}
			}

			if (args[i]!=NULL)
			{
				if (args[i][1]==62)
				{
					args[i] = NULL;
					strcpy(app,args[i+1]);
					flag3 = 1;
				}
			}
		}
		if (flag1)
		{
			int fd;
			fd = open(in,O_RDONLY,0);
			if (fd<0)
			{perror("Could not open input file"); exit(0);}

			dup2(fd,0);
			close(fd);
		}	

		if (flag2)
		{
			struct stat buf;
			if (stat(out,&buf)==0)
			{
				int fo;	
				fo = open(out, O_WRONLY);
				if (fo<0)
				{perror("Could not open output file"); exit(0);}
				dup2(fo,1);
				close(fo);
			}
			else
			{
				int fo;
				fo = creat(out,0644);
				if (fo<0)
				{perror("Could not create output file"); exit(0);}
				dup2(fo,1);
				close(fo);
			}
		}

		if (flag3)
		{
			struct stat buffer;
			if (stat(app,&buffer)==0)
			{
				int fa;
				fa = open(app,O_APPEND | O_WRONLY);
				if (fa<0)
				{perror("Could not open output file"); exit(0);}
				dup2(fa,1);
				close(fa);
			}
			else
			{
				int fa;
				fa = creat(app,0644);
				if (fa<0)
				{perror("Could not create output file"); exit(0);}
				dup2(fa,1);
				close(fa);
			}
		}		
		if (execvp(args[0],args)<0)	
		{r=1; printf("%s: Command doesn't exist\n", args[0]);}
	}
	else
	{wait(NULL);}
	if (r!=1)
	{printf("%s with process id: %d exited normally\n",args[0],pid);}					
	return 1;
}





int shell_setenv (char **args) {
	if (args[2] == NULL) args[2] = " ";
	if (setenv(args[1],args[2],1) != 0) perror("shell");
	return 1;	
}

int shell_unsetenv (char **args) {
	if (unsetenv(args[1]) != 0) perror("shell");
	return 1;
}


int shell_cd (char** args) {
    if (args[1] == NULL)
        return 1;
    else if (strcmp(args[1],"~")==0){
        chdir(root);
    }
    else if (chdir(args[1]) != 0)
        perror("SHELL");
    return 1;
}

int shell_exit (char** args) {
    return EXIT_SUCCESS;
}

int shell_quit (char** args) {
    return EXIT_SUCCESS;
}

int shell_pwd (char** args) {
    char *curr_dir = (char *)malloc(1000*sizeof(char)); 
    getcwd(curr_dir, 1000); 
    printf("%s\n", curr_dir);
    free(curr_dir);
    return 1;
}

int shell_echo (char** args) {
    int i = 1;
    while(args[i] != NULL) {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");  
    return 1;
}

int shell_pinfo (char ** args) {
    char Process[1000];
    strcpy(Process, "/proc/");
    if(args[1]) 
        strcat(Process, args[1]);
    else 
        strcat(Process, "self");
    char Stats[100];
    strcpy(Stats, Process); strcat(Stats, "/stat");
    int error_number[3];
    error_number[0] = 0;
    FILE * stat = fopen(Stats, "r");
    if(error_number[0]) {
        fprintf(stderr, "Error reading %s: %s\n", Stats, strerror(error_number[0]));
        return 1;
    }

    int pid; char status; char name[20];
    fscanf(stat, "%d", &pid); fscanf(stat, "%s", name); fscanf(stat, " %c", &status);
    printf( "pid: %d\n", pid);
    printf( "Process Status: %c\n", status);
    fclose(stat);
    
    error_number[0] = 0;
    strcpy(Stats, Process); strcat(Stats, "/statm");
    FILE * mem = fopen(Stats, "r");
    
    if(error_number[0])     {
	fprintf(stderr, "Error reading %s: %s\n", Stats, strerror(error_number[0]));
        return 1;
    }
    int memSize; fscanf(mem, "%d", &memSize);
    fprintf(stdout, "Memory: %d\n", memSize);
    fclose(mem);
    char exePath[1000];
    strcpy(Stats, Process); strcat(Stats, "/exe");
    error_number[0] = 0;

    readlink(Stats, exePath, sizeof(exePath));
    if(error_number[0]) {
        fprintf(stderr, "Error reading symbolic link %s: %s\n", Stats, strerror(error_number[0]));
        return 1;
    }

    int sameChars = 0, baseL = strlen(root);
    for(sameChars = 0; sameChars < baseL; sameChars++)
        if(root[sameChars] != exePath[sameChars]) break;;
    
    char relPath[1000];
    if(sameChars == baseL) {
        relPath[0] = '~'; relPath[1] = '\0';
        strcat(relPath, (const char *)&exePath[baseL]);
    }
    else {
        strcpy(relPath, exePath);
        relPath[strlen(exePath)] = '\0';
    }

    int i = 0;
    while(exePath[i]) exePath[i++] = '\0';
    
    fprintf(stdout, "Executable Path: %s\n", relPath);
    return 1;
}

int shell_ls (char** args) {
	DIR* dir;
    struct stat mystat[4];
    struct passwd *user[2];
	struct group *group[2];
    char *buffer;
    int count[6];
    count[1] = 0;
    count[2] = 0;
    count[3] = 0;
    count[4] = 1;
	count[5] = 0;
	unsigned char mod[13];
	struct tm *starttime[2];
	time_t now;
	int year; 
    struct dirent **dient[4];
    buffer = malloc(1024*sizeof(char));
	while(args[count[4]] != NULL) {
		count[5] = 0;
		if (args[count[4]][count[5]] == 45) {
			count[5] = 1;
			while(args[count[4]][count[5]] != 0) {
				if(args[count[4]][count[5]] == 108) count[1] = 1;
				if(args[count[4]][count[5]] == 97)  count[2] = 1;
				count[5]++;
			}
		}
		else {
			if (count[4] > 0) count[3] = count[4];
		}
		count[4]++; 
	}
	if (count[3] == 0 || args[count[3]] == NULL) args[count[3]] = ".";
		count[0] = scandir(".",&dient[0],NULL,alphasort);		
	count[0] = scandir(args[count[3]],&dient[0],NULL,alphasort);
	if (count[0] < 0) perror("shell");
	else {
		for(count[4]=0;count[4]<count[0];count[4]++) {
			if (count[1] == 1) {
				if (count[2] == 1) {
					sprintf(buffer,"%s/%s",args[count[3]],dient[0][count[4]]->d_name);
					stat(buffer,&mystat[0]);
					printf( (S_ISDIR(mystat[0].st_mode)) ? "d" : "-");
					printf( (mystat[0].st_mode & S_IRUSR) ? "r" : "-");
					printf( (mystat[0].st_mode & S_IWUSR) ? "w" : "-");
					printf( (mystat[0].st_mode & S_IXUSR) ? "x" : "-");
					printf( (mystat[0].st_mode & S_IRGRP) ? "r" : "-");
					printf( (mystat[0].st_mode & S_IWGRP) ? "w" : "-");
					printf( (mystat[0].st_mode & S_IXGRP) ? "x" : "-");
					printf( (mystat[0].st_mode & S_IROTH) ? "r" : "-");
					printf( (mystat[0].st_mode & S_IWOTH) ? "w" : "-");
					printf( (mystat[0].st_mode & S_IXOTH) ? "x" : "-");
					printf(" \t%d",(int)mystat[0].st_nlink);
					user[1] = getpwuid(mystat[0].st_uid);
						printf(" \t%s", user[1]->pw_name);
					group[1] = getgrgid(mystat[0].st_gid);
						printf(" \t%s", group[1]->gr_name);
					printf(" \t%lld",(long long)mystat[0].st_size);
					time(&now);
					year = localtime(&now)->tm_year;
					starttime[1] = localtime(&mystat[0].st_ctime);
					if(starttime[1]->tm_year == year)
						strftime(mod,13,"%b %e %R",starttime[1]);
					else
						strftime(mod,13,"%b %e %Y",starttime[1]);
					printf(" \t%s",mod );	 	
					printf(" \t%s\n",dient[0][count[4]]->d_name);
				}
				else {
					if ((dient[0][count[4]] ->d_name)[0] != 46) {
						sprintf(buffer,"%s/%s",args[count[3]],dient[0][count[4]]->d_name);
						stat(buffer,&mystat[0]);
						printf( (S_ISDIR(mystat[0].st_mode)) ? "d" : "-");
						printf( (mystat[0].st_mode & S_IRUSR) ? "r" : "-");
						printf( (mystat[0].st_mode & S_IWUSR) ? "w" : "-");
						printf( (mystat[0].st_mode & S_IXUSR) ? "x" : "-");
						printf( (mystat[0].st_mode & S_IRGRP) ? "r" : "-");
						printf( (mystat[0].st_mode & S_IWGRP) ? "w" : "-");
						printf( (mystat[0].st_mode & S_IXGRP) ? "x" : "-");
						printf( (mystat[0].st_mode & S_IROTH) ? "r" : "-");
						printf( (mystat[0].st_mode & S_IWOTH) ? "w" : "-");
						printf( (mystat[0].st_mode & S_IXOTH) ? "x" : "-");
						printf(" \t%d",(int)mystat[0].st_nlink);
						user[1] = getpwuid(mystat[0].st_uid);
						printf(" \t%s", user[1]->pw_name);
						group[1]=getgrgid(mystat[0].st_gid);
						printf(" \t%s", group[1]->gr_name);
						printf(" \t%lld",(long long)mystat[0].st_size);
						time(&now);
						year = localtime(&now)->tm_year;
						starttime[1] = localtime(&mystat[0].st_ctime);
						if(starttime[1]->tm_year == year)
							strftime(mod,13,"%b %e %R",starttime[1]);
						else
							strftime(mod,13,"%b %e %Y",starttime[1]);
						printf(" \t%s",mod );
						printf(" \t%ld",mystat[0].st_mtime);
						printf(" \t%s\n",dient[0][count[4]]->d_name);
					}	 
				}
			}	
			else {
				if (count[2] == 1) {
					printf(" %s\n",dient[0][count[4]]->d_name);	
				}
				else {
					if ((dient[0][count[4]] ->d_name)[0] != 46) printf(" %s\n",dient[0][count[4]]->d_name);
				}
			}	
			free(dient[0][count[4]]);
			
		}
		free(dient[0]);
	}	
	return 1; 

}

char* returnPath (char* cwd) {
    int i,cwd_size = strlen(cwd), root_size = strlen(root);
    if (root_size > cwd_size) {
        return cwd;
    }
    else if (root_size == cwd_size) {
        return "~";
    }
    else {
        char *new = (char*)malloc(100);
        new[0] = '~';
        new[1] = '/';
        for (i = 0 ; i < cwd_size-root_size-1; i++) {
            new[i+2] = cwd[root_size+i+1];
        }
        return new;
    }
} 

void printPrompt (char *root) {
    char hostname[1024];
    char cwd[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    char *username = getenv("USER");
    getcwd(cwd, sizeof(cwd));
    char *path = returnPath(cwd);
    printf("<%s@%s:%s/> ",username,hostname,path);
}

char *readCommands (void) {

    int buffer_size = 1024,check, pos = 0, i;
    char *buffer = malloc(sizeof(char) * buffer_size);
  
    if (!buffer) {
      fprintf(stderr, "ALLOCATION ERROR\n");
      exit(EXIT_FAILURE);
    }
  
    for (i = 0; ;i++) {
      check = getchar();
      if (check == EOF || check == '\n') {
        buffer[pos] = '\0';
        return buffer;
      } 
      else
        buffer[pos] = check;
      
    pos++;
  
      if (pos >= buffer_size) {
        buffer_size += 1024;
        buffer = realloc(buffer, buffer_size);
        if (!buffer) {
          fprintf(stderr, "ALLOCATION ERROR\n");
          exit(EXIT_FAILURE);
        }
      }
    }
}

char **splitLine (char* line) {
    
    int buffer_size = COMM_BUFFER;
    int position  = 0, i = 0;
    char **commands = malloc(buffer_size * (sizeof(char*)));
    char *command;

    if (!commands) {
        fprintf(stderr, "Allocaiton Error \n");
        exit(EXIT_FAILURE);
    }

    command  = strtok(line, COMM_DELIM);
    for (i = 0;; i++) {
        if (command!= NULL) {
        commands[position++] = command;
            if (buffer_size < position) {
                buffer_size += COMM_BUFFER;
                commands = realloc(commands, buffer_size * sizeof(char*));
                if (!commands) {
                    fprintf(stderr, "Allocaiton Error \n");
                    exit(EXIT_FAILURE);
                }
            }   
            command = strtok(NULL, COMM_DELIM);
            continue;
        }
        break;
    }
    commands[position] = NULL;
    return commands;
}


char **splitCommand (char* line) {
    
    int buffer_size = TOK_BUFFER;
    int position  = 0, i = 0;
    char **tokens = malloc(buffer_size * (sizeof(char*)));
    char *token;

    if (!tokens) {
        fprintf(stderr, "Allocaiton Error \n");
        exit(EXIT_FAILURE);
    }

    token  = strtok(line, TOK_DELIM);
    for (i = 0;; i++) {
        if (token!= NULL) {
        tokens[position++] = token;
            if (buffer_size < position) {
                buffer_size += TOK_BUFFER;
                tokens = realloc(tokens, buffer_size * sizeof(char*));
                if (!tokens) {
                    fprintf(stderr, "Allocaiton Error \n");
                    exit(EXIT_FAILURE);
                }
            }   
            token = strtok(NULL, TOK_DELIM);
            continue;
        }
        break;
    }
    tokens[position] = NULL;
    return tokens;
}

int launch (char** args) {
    
    int i,j,background = 0, redirectflag = 0, piping = 0;
    for (i = 0;args[i] != NULL;i++) {
        for (j = 0;args[i][j] != '\0'; j++) {
            if (args[i][j] == '>' || args[i][j] == '<') {
                redirectflag = 1;
            }
            if (args[i][j] == '|') {
                piping = 1;
            }
            if (args[i][j] == '&') {
                background = 1;
            }
        }
    }
    if (piping) {
        pipehandler(args);
        return 1;
    }
    if (redirectflag) {
        return redirect(args, i);   
    }
    pid_t pid, wpid;
    int state;
    pid = fork();
    
    if (pid < 0) {
        perror("ERROR");
    }

    else if(!pid) {
        if ( execvp(args[0], args) == -1)
            perror("ERROR");
        exit(EXIT_FAILURE);
    }
    if(!background) {
        wpid = waitpid (pid, &state, WUNTRACED);
        for (i = 0;; i++) {
        if (!WIFEXITED(state) && !WIFSIGNALED(state)) {
            wpid = waitpid (pid, &state, WUNTRACED);
            continue;
            }
        break;
        }
    }
    return 1;
}

int checkCommand (char** args) {
    int count;
    if (args[0] == NULL)
        return EXIT_FAILURE;

    count = 0;
    while (count < sizeof(builtin_arr)/sizeof(char*)) {
        if (strcmp(args[0],builtin_arr[count]) == 0)
            return (*builtin_functions[count])(args);
        
        count++;
    }
    return launch(args);
}

void interpretCommand(void) {
    char **args;
    char *commands;
    char **split_line;
    int state;
    int i,j;
    signal(SIGINT,sigintHandler);
    for (i = 0;; i++) {
        printPrompt(root);
        commands  = readCommands();
        split_line = splitLine(commands);
        for (j = 0;split_line[j] != NULL ;j++) {
            args = splitCommand(split_line[j]);
            state = checkCommand(args);
            free (args);
            if (!state) {
                break;
            }
        }
        free(commands);
        free(split_line);
        if(!state) {
            break;
        }
    }
}

int main(int arg1, char **arg2) {
    root = getenv("PWD");
    interpretCommand();
    return EXIT_SUCCESS;
}