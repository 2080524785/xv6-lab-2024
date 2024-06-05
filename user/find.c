#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
void
find(char *dir, char *name)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(dir, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", dir);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", dir);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(strcmp(dir + strlen(dir) - strlen(name), name)==0){
        printf("%s\n", dir);
    }
    break;

  case T_DIR:
    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, dir);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      
      if(strcmp(".", de.name)!=0 && strcmp("..", de.name)!=0){
        find(buf,name);
      }
    }
    break;
  }
  close(fd);
}
int main(int argc, char *argv[]){
    if(argc!=3){
        fprintf(2,"Usage: find <dir> <name>...\n");
        exit(1);
    }
    char *dir = argv[1], *name = argv[2];
    find(dir, name);
    exit(0);
}