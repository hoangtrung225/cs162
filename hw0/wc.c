#include <stdio.h>
int main(int argc, char *argv[]) {
  char* errorMessage = "usage: wc filename";
  if(argc != 2)
  {
    printf("%s", errorMessage);
    return 1;
  }
  FILE* file = fopen(argv[1],"r");
  if(file == NULL)
  {
    printf("cant open file %s", errorMessage);
    return 1;
  }
  char readChar;
  int counter = 0;
  do {
    /* code */
    readChar = fgetc(file);
    counter++;
  }
  while(readChar != EOF);
  printf("character in file: %d", counter -1);
  return 0;
}
