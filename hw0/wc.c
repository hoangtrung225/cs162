#include <stdio.h>
#include <ctype.h>
int main(int argc, char *argv[]) {
    char* usage_message = "usage: wc FILENAME";
    if(argc != 2){
      printf("%s", usage_message);
      return 1;
    }
    FILE* file_open = fopen(argv[1],"r");
    if(file_open == NULL){
      printf("cant open %s", argv[1]);
      return 1;
    }

    int byte = 0;
    int new_line = 0;
    int word_count =0;
    char newchar, prechar;
    while(newchar != EOF){
      prechar = newchar;
      newchar = fgetc(file_open);
      if(newchar != EOF) byte++;
      if((newchar == ' ' || newchar == '\n') && (32 < prechar && prechar < 177)) word_count++;
      if(newchar == '\n') new_line++;
    }

    printf("%d  %d  %d", new_line, word_count, byte);
    return 0;
}
