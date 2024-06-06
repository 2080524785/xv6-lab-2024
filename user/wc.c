#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char buf[512];
int output_lines = 0;
void
wc(int fd, char *name)
{
  int i, n;
  int l, w, c, inword;

  l = w = c = 0;
  inword = 0;
  while((n = read(fd, buf, sizeof(buf))) > 0){
    for(i=0; i<n; i++){
      c++;
      if(buf[i] == '\n')
        l++;
      if(strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if(!inword){
        w++;
        inword = 1;
      }
    }
  }
  if(n < 0){
    printf("wc: read error\n");
    exit(1);
  }
  if(output_lines == 0)
    printf("%d %d %d %s\n", l, w, c, name);
  else
    printf("%d\n", l);
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if(argc <= 1){
    fprintf(2, "usage: wc [-l] file\n");
    exit(1);
  }

  if(argc>2 && strcmp(argv[1], "-l") == 0){
    output_lines = 1;
  }

  for(i = 1+output_lines; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf("wc: cannot open %s\n", argv[i]);
      exit(1);
    }
    wc(fd, argv[i]);
    close(fd);
  }
  exit(0);
}
