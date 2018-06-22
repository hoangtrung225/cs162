
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "libhttp.h"

int NAMESIZE = 256;
int BUFSIZE = 512;

void handle_request_file(int fd, char* file_location)
{
  int file_fd;
  if((file_fd = open(file_location, O_RDONLY)) < 0){
    http_start_response(fd, 400);
    http_end_headers(fd);
    return;
  }
  printf("request file: %s\n", file_location);
  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", http_get_mime_type(file_location));

  struct stat stat_file;
  if(stat(file_location, &stat_file) == -1) {
    perror("failed to get file stat");
    http_start_response(fd, 501);
    http_end_headers(fd);
  }

  char contentlen[20];
  snprintf(contentlen, 20, "%ld", stat_file.st_size);
  http_send_header(fd, "Content-Length", contentlen);
  http_end_headers(fd);

  int READBUFSIZE = 1024;
  char read_buffer[READBUFSIZE];
  int buf_read;
  while((buf_read = read(file_fd, read_buffer, READBUFSIZE)) > 0){
    http_send_data(fd, read_buffer, buf_read);
  }
  close(file_fd);
}
void handle_request_dir(int fd, char* dir_location){
  DIR* request_dir = opendir(dir_location);
  if(request_dir == NULL){
    http_start_response(fd, 400);
    http_end_headers(fd);
    return;
  }
  printf("request directory: %s\n", dir_location);
  chdir(dir_location);

  struct dirent *dir_struct;
  int entries_num = 0;
  char** files_ptr = malloc((entries_num + 1)*sizeof(char*));
  while((dir_struct = readdir(request_dir)) != NULL){
    files_ptr[entries_num] = malloc(NAMESIZE);
    snprintf(files_ptr[entries_num], NAMESIZE, "%s", dir_struct->d_name);

    entries_num++ ;
    files_ptr = realloc(files_ptr, (entries_num + 1)*sizeof(char*));
    files_ptr[entries_num] = '\0';
    //serve index.html if existing
    if(strcmp(dir_struct->d_name, "index.html") == 0){
      handle_request_file(fd, "index.html");
      for(int i = 0; i < entries_num; i++)  free(files_ptr[i]);

      free(files_ptr);
      return;
      }
  }

  http_start_response(fd, 200);
  http_send_header(fd, "Content-Type", "text/html");
  http_end_headers(fd);
  char* string_send = malloc(BUFSIZE);
  int max_string_len;
  snprintf(string_send, BUFSIZE, "<b><i>");
  for(int i = 0; i < entries_num; i++){
    max_string_len = BUFSIZE + (i+1)*BUFSIZE;
    string_send = realloc(string_send, max_string_len);
    snprintf(string_send + strlen(string_send), max_string_len, "<a href=\"%s\">%s</a></br>", files_ptr[i], files_ptr[i]);
  }
  snprintf(string_send + strlen(string_send), max_string_len, "</i></b>");
  http_send_string(fd, string_send);
  //free files_ptr
  for(int i = 0; i < entries_num; i++)  free(files_ptr[i]);
  free(string_send);
  close(fd);
}
