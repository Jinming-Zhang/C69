#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv){
  int lst[50];
  char *this = "on the heap";
  char *stk[20];
  lst[2] = 20;
  int *hp = malloc(sizeof(int));
  *hp = 10;

  return 0;
}
