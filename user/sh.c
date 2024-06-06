#include "kernel/types.h"    // 包含类型定义头文件
#include "user/user.h"       // 包含用户级别系统调用和库函数头文件
#include "kernel/fcntl.h"    // 包含文件控制头文件
#include "kernel/stat.h"
#include "kernel/fs.h"

// 命令类型定义
#define EXEC  1    // 执行命令
#define REDIR 2    // 重定向命令
#define PIPE  3    // 管道命令
#define LIST  4    // 命令列表
#define BACK  5    // 后台命令

#define MAXARGS 10    // 最大参数数量

// 命令结构定义
struct cmd {
  int type;    // 命令类型
};

// 执行命令结构定义
struct execcmd {
  int type;                 // 命令类型 (EXEC)
  char *argv[MAXARGS];      // 参数列表
  char *eargv[MAXARGS];     // 参数末尾列表
};

// 重定向命令结构定义
struct redircmd {
  int type;                 // 命令类型 (REDIR)
  struct cmd *cmd;          // 子命令
  char *file;               // 文件名
  char *efile;              // 文件名末尾
  int mode;                 // 打开模式
  int fd;                   // 文件描述符
};

// 管道命令结构定义
struct pipecmd {
  int type;                 // 命令类型 (PIPE)
  struct cmd *left;         // 左子命令
  struct cmd *right;        // 右子命令
};

// 命令列表结构定义
struct listcmd {
  int type;                 // 命令类型 (LIST)
  struct cmd *left;         // 左子命令
  struct cmd *right;        // 右子命令
};

// 后台命令结构定义
struct backcmd {
  int type;                 // 命令类型 (BACK)
  struct cmd *cmd;          // 子命令
};

// 函数声明
int fork1(void);                       // fork 的包装函数
void panic(char*);                     // 错误处理函数
struct cmd *parsecmd(char*);           // 命令解析函数

// 运行命令函数
void runcmd(struct cmd *cmd)
{
  int p[2];                             // 管道文件描述符数组
  struct backcmd *bcmd;                 // 后台命令指针
  struct execcmd *ecmd;                 // 执行命令指针
  struct listcmd *lcmd;                 // 命令列表指针
  struct pipecmd *pcmd;                 // 管道命令指针
  struct redircmd *rcmd;                // 重定向命令指针

  if(cmd == 0)                          // 如果命令为空
    exit(1);                            // 退出

  switch(cmd->type){                    // 根据命令类型执行相应操作
  default:
    panic("runcmd");                    // 未知命令类型

  case EXEC:
    ecmd = (struct execcmd*)cmd;        // 转换为执行命令结构
    if(ecmd->argv[0] == 0)              // 如果没有可执行的命令
      exit(1);                          // 退出
    exec(ecmd->argv[0], ecmd->argv);    // 执行命令
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);  // 执行失败
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;       // 转换为重定向命令结构
    close(rcmd->fd);                    // 关闭原文件描述符
    if(open(rcmd->file, rcmd->mode) < 0){  // 打开重定向文件
      fprintf(2, "open %s failed\n", rcmd->file);  // 打开失败
      exit(1);                          // 退出
    }
    runcmd(rcmd->cmd);                  // 执行子命令
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;        // 转换为命令列表结构
    if(fork1() == 0)                    // 创建子进程执行左子命令
      runcmd(lcmd->left);               // 执行左子命令
    wait(0);                            // 等待左子命令完成
    runcmd(lcmd->right);                // 执行右子命令
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;        // 转换为管道命令结构
    if(pipe(p) < 0)                     // 创建管道
      panic("pipe");                    // 创建失败
    if(fork1() == 0){                   // 创建子进程执行左子命令
      close(1);                         // 关闭标准输出
      dup(p[1]);                        // 复制管道写端到标准输出
      close(p[0]);                      // 关闭管道读端
      close(p[1]);                      // 关闭管道写端
      runcmd(pcmd->left);               // 执行左子命令
    }
    if(fork1() == 0){                   // 创建子进程执行右子命令
      close(0);                         // 关闭标准输入
      dup(p[0]);                        // 复制管道读端到标准输入
      close(p[0]);                      // 关闭管道读端
      close(p[1]);                      // 关闭管道写端
      runcmd(pcmd->right);              // 执行右子命令
    }
    close(p[0]);                        // 关闭管道读端
    close(p[1]);                        // 关闭管道写端
    wait(0);                            // 等待左子命令完成
    wait(0);                            // 等待右子命令完成
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;        // 转换为后台命令结构
    if(fork1() == 0)                    // 创建子进程
      runcmd(bcmd->cmd);                // 后台执行子命令
    break;
  }
  exit(0);                              // 退出进程
}

int script_fd=-1;
int is_valid_identifier_char(char c) { // used in tab completion
  return  (c >= 'A' && c <= 'Z') ||
          (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') ||
          c == '_' || c == '.' || c == '-';
}

char *find_last_word(char *buf) {
  char *word = buf + strlen(buf) - 1;
  while(is_valid_identifier_char(*word) && word != buf-1) {
    word--;
  }
  word++;
  return word;
}

int tab_completion(char *cmdbuf) {
  char *last_word = find_last_word(cmdbuf);
  // printf("\ntrying tab:%s\n", word);

  // copied from find.c
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

  const char* path = ".";
	if((fd = open(path, 0)) < 0){
		fprintf(2, "auto-complete: cannot open %s\n", path);
		return 0;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "auto-complete: cannot stat %s\n", path);
		close(fd);
		return 0;
	}

  int added_length = 0;
	switch(st.type){
	case T_DIR:
		// printf("%s\n", path);
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("auto-complete: path too long\n");
			break;
		}
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("auto-complete: cannot stat %s\n", buf);
				continue;
			}
	//   printf("%s\n", buf);
			if(strcmp(buf+strlen(buf)-2, "/.") != 0 && strcmp(buf+strlen(buf)-3, "/..") != 0) {
				char *last_word_from_buf = find_last_word(buf);
        if(memcmp(last_word_from_buf, last_word, strlen(last_word)) == 0) {
          printf("auto-completed: %s\n\n", last_word_from_buf);
          added_length = strlen(last_word_from_buf) - strlen(last_word);
          strcpy(last_word, last_word_from_buf);
          break;
        }
			}
		}
		break;
	}
	close(fd);
  return added_length;
}
// 获取命令输入函数
int getcmd(char *buf, int nbuf)
{
  if(script_fd==-1){
    fprintf(2, "$ ");
  }                       // 输出提示符
  memset(buf, 0, nbuf);                 // 清空缓冲区
  // gets(buf, nbuf);                      // 获取用户输入

  int i, cc;
  char c;

  for(i=0;i<nbuf-1;){
    if(script_fd==-1){
      cc = read(0, &c, 1);
    }else{
      cc = read(script_fd, &c, 1);
    }

    if(cc<1) break;

    if(c=='\t'){
      // i+= tab_completion(buf);
    }else{
      buf[i++] = c;
      if(c=='\n' || c == '\r') break;
    }
  }
  buf[i++] = '\0';

  if(buf[0] == 0)                       // 如果输入为空
    return -1;                          // 返回 -1
  return 0;                             // 返回 0
}

void process_cmd(char *buf){
  if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){  // 处理 cd 命令
    buf[strlen(buf)-1] = 0;           // 去掉换行符
    if(chdir(buf+3) < 0)              // 改变目录
      fprintf(2, "cannot cd %s\n", buf+3);  // 错误处理
    return;                         // 继续下一个循环
  }

  if(strcmp(buf, "wait\n")==0){
    while(wait(0)>0);
    printf("(No pid is running now.)\n");
    return;
  }

  if(fork1() == 0)                    // 创建子进程执行命令
    runcmd(parsecmd(buf));            // 解析并运行命令
  wait(0);
}
// 主函数
int main(int argc, char **argv)
{
  static char buf[100];                 // 命令缓冲区
  int fd;                               // 文件描述符
  
  if(argc >= 2) { // miigon: added support for running script from a file
    script_fd = open(argv[1], O_RDONLY);
    if (script_fd < 0) {
      fprintf(2, "cannot open %s\n", argv[1]);
      exit(1);
    }
  }

  // printf("sh:start\n");
  while((fd = open("console", O_RDWR)) >= 0){  // 打开控制台
    if(fd >= 3){                        // 如果文件描述符大于等于 3
      close(fd);                        // 关闭文件描述符
      break;                            // 退出循环
    }
  }

  

  while(getcmd(buf, sizeof(buf)) >= 0){  // 获取命令输入
    char *cmdstart = buf, *p = buf;
    while(*p!='\0'){
      if(*p == ';'){
        *p = '\0';
        process_cmd(cmdstart);
        cmdstart = p+1;
      }
      p++;
    }
    process_cmd(cmdstart);
  }
  
  exit(0);                              // 退出进程
}

// 错误处理函数
void panic(char *s)
{
  fprintf(2, "%s\n", s);                // 输出错误信息
  exit(1);                              // 退出进程
}

// fork 包装函数
int fork1(void)
{
  int pid;
  pid = fork();                         // 创建子进程
  if(pid == -1)                         // 如果创建失败
    panic("fork");                      // 调用错误处理函数
  return pid;                           // 返回子进程 ID
}

// 创建执行命令结构体
struct cmd* execcmd(void)
{
  struct execcmd *cmd;
  cmd = malloc(sizeof(*cmd));           // 分配内存
  memset(cmd, 0, sizeof(*cmd));         // 清零内存
  cmd->type = EXEC;                     // 设置命令类型为 EXEC
  return (struct cmd*)cmd;              // 返回命令指针
}

// 创建重定向命令结构体
struct cmd* redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;
  cmd = malloc(sizeof(*cmd));           // 分配内存
  memset(cmd, 0, sizeof(*cmd));         // 清零内存
  cmd->type = REDIR;                    // 设置命令类型为 REDIR
  cmd->cmd = subcmd;                    // 设置子命令
  cmd->file = file;                     // 设置文件名
  cmd->efile = efile;                   // 设置文件名末尾
  cmd->mode = mode;                     // 设置打开模式
  cmd->fd = fd;                         // 设置文件描述符
  return (struct cmd*)cmd;              // 返回命令指针
}

// 创建管道命令结构体
struct cmd* pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;
  cmd = malloc(sizeof(*cmd));           // 分配内存
  memset(cmd, 0, sizeof(*cmd));         // 清零内存
  cmd->type = PIPE;                     // 设置命令类型为 PIPE
  cmd->left = left;                     // 设置左子命令
  cmd->right = right;                   // 设置右子命令
  return (struct cmd*)cmd;              // 返回命令指针
}

// 创建命令列表结构体
struct cmd* listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;
  cmd = malloc(sizeof(*cmd));           // 分配内存
  memset(cmd, 0, sizeof(*cmd));         // 清零内存
  cmd->type = LIST;                     // 设置命令类型为 LIST
  cmd->left = left;                     // 设置左子命令
  cmd->right = right;                   // 设置右子命令
  return (struct cmd*)cmd;              // 返回命令指针
}

// 创建后台命令结构体
struct cmd* backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;
  cmd = malloc(sizeof(*cmd));           // 分配内存
  memset(cmd, 0, sizeof(*cmd));         // 清零内存
  cmd->type = BACK;                     // 设置命令类型为 BACK
  cmd->cmd = subcmd;                    // 设置子命令
  return (struct cmd*)cmd;              // 返回命令指针
}

char whitespace[] = " \t\r\n\v";        // 空白字符
char symbols[] = "<|>&;()";             // 特殊符号

int gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;                              // 设置当前指针
  while(s < es && strchr(whitespace, *s))  // 跳过空白字符
    s++;
  if(q)
    *q = s;                             // 设置参数起始指针
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;                            // 设置参数末尾指针

  while(s < es && strchr(whitespace, *s))  // 跳过空白字符
    s++;
  *ps = s;                              // 更新指针
  return ret;                           // 返回标记类型
}

int peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))  // 跳过空白字符
    s++;
  *ps = s;
  return *s && strchr(toks, *s);        // 返回是否是目标字符
}

struct cmd* parseline(char**, char*);
struct cmd* parsepipe(char**, char*);
struct cmd* parseexec(char**, char*);
void nulterminate(struct cmd *cmd);

struct cmd* parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);                   // 设置结束指针
  cmd = parseline(&s, es);              // 解析整行命令
  peek(&s, es, "");                     // 检查是否有剩余字符
  if(s != es){
    fprintf(2, "leftovers: %s\n", s);   // 输出剩余字符
    panic("syntax");                    // 语法错误处理
  }
  nulterminate(cmd);                    // 终止字符串
  return cmd;                           // 返回命令结构
}

struct cmd* parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);              // 解析管道命令
  while(peek(ps, es, "&")){             // 处理后台命令
    gettoken(ps, es, 0, 0);             // 获取标记
    cmd = backcmd(cmd);                 // 创建后台命令
  }
  if(peek(ps, es, ";")){                // 处理命令列表
    gettoken(ps, es, 0, 0);             // 获取标记
    cmd = listcmd(cmd, parseline(ps, es));  // 创建命令列表
  }
  return cmd;                           // 返回命令结构
}

struct cmd* parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);              // 解析执行命令
  if(peek(ps, es, "|")){                // 处理管道命令
    gettoken(ps, es, 0, 0);             // 获取标记
    cmd = pipecmd(cmd, parsepipe(ps, es));  // 创建管道命令
  }
  return cmd;                           // 返回命令结构
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){            // 处理重定向命令
    tok = gettoken(ps, es, 0, 0);       // 获取标记
    if(gettoken(ps, es, &q, &eq) != 'a')  // 获取文件名
      panic("missing file for redirection");  // 处理错误
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);  // 创建输入重定向命令
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE|O_TRUNC, 1);  // 创建输出重定向命令
      break;
    case '+':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);  // 创建追加重定向命令
      break;
    }
  }
  return cmd;                           // 返回命令结构
}

struct cmd* parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))                // 检查是否是块命令
    panic("parseblock");                // 处理错误
  gettoken(ps, es, 0, 0);               // 获取标记
  cmd = parseline(ps, es);              // 解析行命令
  if(!peek(ps, es, ")"))                // 检查是否闭合
    panic("syntax - missing )");        // 处理错误
  gettoken(ps, es, 0, 0);               // 获取标记
  cmd = parseredirs(cmd, ps, es);       // 解析重定向
  return cmd;                           // 返回命令结构
}

struct cmd* parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))                 // 检查是否是块命令
    return parseblock(ps, es);          // 解析块命令

  ret = execcmd();                      // 创建执行命令结构
  cmd = (struct execcmd*)ret;           // 转换为执行命令结构

  argc = 0;                             // 初始化参数计数
  ret = parseredirs(ret, ps, es);       // 解析重定向
  while(!peek(ps, es, "|)&;")){         // 检查是否是结束标记
    if((tok=gettoken(ps, es, &q, &eq)) == 0)  // 获取标记
      break;
    if(tok != 'a')                      // 如果不是参数
      panic("syntax");                  // 处理错误
    cmd->argv[argc] = q;                // 设置参数
    cmd->eargv[argc] = eq;              // 设置参数末尾
    argc++;                             // 增加参数计数
    if(argc >= MAXARGS)                 // 如果参数过多
      panic("too many args");           // 处理错误
    ret = parseredirs(ret, ps, es);     // 解析重定向
  }
  cmd->argv[argc] = 0;                  // 设置参数结束
  cmd->eargv[argc] = 0;                 // 设置参数末尾结束
  return ret;                           // 返回命令结构
}

void nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)                          // 如果命令为空
    return;

  switch(cmd->type){                    // 根据命令类型执行相应操作
  case EXEC:
    ecmd = (struct execcmd*)cmd;        // 转换为执行命令结构
    for(i=0; ecmd->argv[i]; i++)        // 处理每个参数
      *ecmd->eargv[i] = 0;              // 终止字符串
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;       // 转换为重定向命令结构
    nulterminate(rcmd->cmd);            // 终止子命令字符串
    *rcmd->efile = 0;                   // 终止文件名字符串
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;        // 转换为管道命令结构
    nulterminate(pcmd->left);           // 终止左子命令字符串
    nulterminate(pcmd->right);          // 终止右子命令字符串
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;        // 转换为命令列表结构
    nulterminate(lcmd->left);           // 终止左子命令字符串
    nulterminate(lcmd->right);          // 终止右子命令字符串
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;        // 转换为后台命令结构
    nulterminate(bcmd->cmd);            // 终止子命令字符串
    break;
  }
}
