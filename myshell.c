#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}
char error_message[30] = "An error has occurred\n";
void error() {
  write(STDOUT_FILENO, error_message, strlen(error_message));
}
int whitespace(char *s) {
  if(s == NULL) {
    return 1;
  }
  int i = 0;
  while(1) {
    if(s[i] == '\0') {
      return 1;
    } else if (!isspace(s[i])) {
      return 0;
    } else {
      i++;
    }
  }
}
struct strnode {
  char *word;
  struct strnode *next;
};
typedef struct strnode strnode_t;
strnode_t *new_node(char* word, strnode_t *ls) {
  strnode_t *nnode = malloc(sizeof(strnode_t));
  nnode->word = word;
  nnode->next = ls;
  return nnode;
} 
int valid_word(char *s) {
  return (s != NULL) && !whitespace(s) && (s[0] != '\0');
}
int redir(char *s,int *first_char_redir,int *advanced) {
  if(s == NULL) {
    return 0;
  }
  int i = 0;
  while(s[i] != '\0') {
    if(s[i] == '>') {
      if(i==0) {
        *first_char_redir = 1;
      } else {
        *first_char_redir = 0;
      }
      if(s[i+1] == '+') {
        *advanced = 1;
      } else {
        *advanced = 0;
      }
      return 1;
    }
    i++;
  }
  return 0;
}
    
int main(int argc, char *argv[]) 
{
  char *pinput;
  
  char *curr_comm;
  int num_comms = 0;
  int *comm_lens;
  
  char *curr_word;
  char *subword;
  int argcc = 0;
  char **argvc;
  strnode_t *argarr = NULL;
  
  int batch = 0;
  int fbatch;
  int validline = 1;
  if(argc > 2) {
    error();
    exit(0);
  }
  if(argc == 2) {
    if(access(argv[1],F_OK) != -1) {
      batch = 1;
      fbatch = open(argv[1],0);
      dup2(fbatch,0);
    } else {
      error();
      exit(0);
    }
  } 
  
  while(1) {
    char cmd_buff[515] = {'\0'};
    validline = 1;
    
    if(!batch) {
      myPrint("myshell> ");
    }
    
    pinput = fgets(cmd_buff, 515, stdin);
    if (!pinput) {
      exit(0);
    }
    if((strlen(cmd_buff) > 513) || (batch && !whitespace(cmd_buff))) {
      write(STDOUT_FILENO,cmd_buff,(strlen(cmd_buff)<514)?strlen(cmd_buff):513);
    }
    if(strlen(cmd_buff) > 513) {
      validline = 0;
      char extra = cmd_buff[513];
      while(extra != '\n') {
        char strextra[2] = {extra,'\0'};
        myPrint(strextra);
        extra = getchar();
      }
      myPrint("\n");
      error();
    }
    
    char *r1;
    curr_comm = strtok_r(pinput,";\n",&r1);
    while((curr_comm != NULL) && validline) {
      if(!whitespace(curr_comm)) {
        char *r2;
        num_comms++;
        curr_word = strtok_r(curr_comm," \n        ",&r2);
        while(valid_word(curr_word)) {
          int *first_char_redir = malloc(sizeof(int));
          int *advanced = malloc(sizeof(int));
          if(redir(curr_word,first_char_redir,advanced) && (strcmp(curr_word,">") != 0)) {
            char *r3;
            
            subword = strtok_r(curr_word,advanced?">+":">",&r3);
            if(valid_word(subword) && !*first_char_redir) {
              argcc++;
              argarr = new_node(subword,argarr);
            }
            
            argcc++;
            char *gr = advanced?">+":">";
            argarr = new_node(gr,argarr);
            
            if(!*first_char_redir) {
              subword = strtok_r(NULL,advanced?">+":">",&r3);
            }
            if(valid_word(subword)) {
              argcc++;
              argarr = new_node(subword,argarr);
            }
          } else {
            argcc++;
            argarr = new_node(curr_word,argarr);
          }
          curr_word = strtok_r(NULL," \n",&r2);
        }
        argcc++;
        argarr = new_node(NULL,argarr);
      }
      curr_comm = strtok_r(NULL,";\n",&r1);
    }
    
    comm_lens = malloc(sizeof(int)*num_comms+1);
    for(int i=0; i<num_comms+1; i++) { 
      comm_lens[i] = 0;
    }
    
    argvc = malloc((argcc+1) * sizeof(char*));
    
    int j = -1;
    for(int i=0; i<argcc; i++) {
      argvc[argcc - 1 - i] = argarr->word;
      if(argvc[argcc - 1 - i] == NULL) {
        j++;
      }
      comm_lens[num_comms - j]++;
      argarr = argarr->next;
    }
    argvc[argcc] = NULL;
    //printf("\n\n");
    //for(int i=0; i<argcc; i++) {
    //if(argvc[i] == NULL) {
    //printf("NULL\n");
    //} else {
    //printf("%s\n",argvc[i]);
    //}
    //}
    //printf("%d\n\n",num_comms);
    //for(int i=0; i<num_comms+1; i++) {
    //printf("%d\n",comm_lens[i]);
    //}
    
    int b = 0;
    
    for(int i=0; (i<num_comms) && (argvc[b] != NULL); i++) {
      int valid_comm = 1;
      b += comm_lens[i];
      int stdout_cpy = dup(1);
      int fdout = 1;
      int r_adv = 0;
      char *fout;
      char *filename;
      int more_comm = 1;
      for(int j=1; (j < comm_lens[i+1]-1) && more_comm; j++) {
        
        if((strcmp(argvc[b+j],">") == 0) || (strcmp(argvc[b+j],">+") == 0)) {
          more_comm = 0;
          if((argvc[b+j+1] == NULL) || (argvc[b+j+2] != NULL) ||
             (strcmp(argvc[b],"exit")==0) || (strcmp(argvc[b],"cd")==0) ||
             (strcmp(argvc[b],"pwd")==0)){
            error();
            valid_comm = 0;
          } else if(access(argvc[b+j+1],F_OK) >= 0) {
            if(strcmp(argvc[b+j],">+") == 0) {
              r_adv = 1;
              fout = argvc[b+j+1];
              filename = "tempfile.txt";
              fdout = creat(filename,S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
              dup2(fdout,1);
              close(fdout);
              argvc[b+j] = NULL;
              valid_comm = 1;
            } else {
              error();
              valid_comm = 0;
            }
          } else {
            fdout = creat(argvc[b+j+1],S_IRWXU);
            if(fdout < 0) {
              error();
              valid_comm = 0;
            } else {
              dup2(fdout,1);
              close(fdout);
              argvc[b+j] = NULL;
              valid_comm = 1;
            }
          }
        }
      }
      
      if(valid_comm) {
        if(strcmp(argvc[b],"exit") == 0) {
          if(argvc[b+1] == NULL) {
            exit(0);
          } else {
            error();
          }
        } else if (strcmp(argvc[b],"cd") == 0) {
          if(argvc[b+1] == NULL) {
            chdir(getenv("HOME"));
          } else if(argvc[b+2] != NULL) {
            error();
          } else if(chdir(argvc[b+1]) < 0) {
            error();
          }
        } else if (strcmp(argvc[b],"pwd") == 0) {
          if(argvc[b+1] == NULL) {
            char BUF[100] = {0};
            getcwd(BUF,100);
            myPrint(BUF);
            write(STDOUT_FILENO,"\n",strlen("\n"));
          } else {
            error();
          }
        } else {
          int forkret = fork();
          if(forkret == 0) {
            if(execvp(argvc[b], argvc + b) < 0) {
              error();
            }
            exit(0);
          } else {
            waitpid(forkret,NULL,0);
          }
        }
        if(r_adv) {
          int real_fdout = open(fout,O_RDWR);
          if(real_fdout < 0) {
            error();
          } else {
            char c[3] = {0};
            int more = read(real_fdout,c,1);
            while(more) {
              myPrint(c);
              more = read(real_fdout,c,1);
            }
            rename(filename,fout);
          }
          close(real_fdout);
        }
        
        dup2(stdout_cpy,1);
      }
      
    }
    
    j = 0;
    num_comms = 0;
    comm_lens = NULL;
    argvc = NULL;
    argcc = 0;
    argarr = NULL;
  }
}
