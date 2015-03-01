#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int mode;          // the mode to open the file with
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd = NULL;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit();
  
  switch(cmd->type){
  default:
    printf("unknown runcmd\n");
    exit();

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();
    //fprintf(stderr, "exec not implemented\n");
    // Your code here ...
    execv(ecmd->argv[0], ecmd->argv);
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    //fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    if(rcmd->fd < 2) {
        //int oo = rcmd->fd;
        close(rcmd->fd);
        rcmd->fd = open(rcmd->file, rcmd->mode);
        //dup2(rcmd->fd, oo);
        runcmd(rcmd->cmd);
    }
    execv(ecmd->argv[0], ecmd->argv);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    //fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    struct execcmd *lcmd;
    //lcmd = (struct execcmd*)pcmd->left;
    //pcmd = (struct pipecmd*)pcmd->right;
    int sp[2];
    pipe(sp);
    if (fork1() == 0) {
        dup2(sp[1], 1);
        runcmd(pcmd->left);
    }
    close(sp[1]);
    dup2(sp[0], 0);
    runcmd(pcmd->right);
    close(sp[0]);
/*
    int r,i = 0,h,p, pp[2][2];
    pipe(pp[1]);
    while(pcmd->type == '|') {
        lcmd = (struct execcmd*)pcmd->left;
        pcmd = (struct pipecmd*)pcmd->right;
        h = i ? 0 : 1;
        close(pp[h][1]);
        pipe(pp[i]);
        if (fork1() == 0) {
            dup2(pp[i][1], 1);
            dup2(pp[h][0], 0);
            //close(pp[i][0]);
            //close(pp[h][1]);
            execvp(lcmd->argv[0], lcmd->argv);
        }
        close(pp[h][0]);
        i = i ? 0 : 1;
    }
    h = i ? 0 : 1;
    lcmd = (struct execcmd*)pcmd;
    close(pp[h][1]);
    dup2(pp[h][0], 0);
    execvp(lcmd->argv[0], lcmd->argv);
    //wait(&r);
*/
    break;
  }
  exit();
}

int
getcmd(char *buf, int nbuf)
{
  
  printf("W> ");
  memset(buf, 0, nbuf);
  read(stdin, buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  memset(buf, 0, 100);
  
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        printf("cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0) {
      runcmd(parsecmd(buf));
    }
    wait();
  }
  exit();
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_RDWR|O_CREAT;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;
  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit();
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit();
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit();
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit();
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
