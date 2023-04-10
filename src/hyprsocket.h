#ifndef HYPRSOCKET_H
#define HYPRSOCKET_H

// extern "C" {

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdlib.h> // setenv, getenv
// cpp
#include <atomic>

static int fd;
static struct sockaddr_un addr;
static char SERVER_SOCK_FILE[100] = {0};
static void setHyprlandSocket(char* sock_file){
  strncpy((char*)sock_file, "/tmp/hypr/", 10);
  strncat((char*)sock_file, getenv("HYPRLAND_INSTANCE_SIGNATURE"), 70);
  strncat((char*)sock_file, "/.socket2.sock", 15);
}

void closeSocketConnection() {
	if (fd >= 0) {
		close(fd);
	}
  printf("Closed Socket\n");
}

int initSocketConnection() {
	int err = 0;

  setHyprlandSocket(SERVER_SOCK_FILE);

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create a socket");
		err = 1;
	}

	if (!err) {
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, SERVER_SOCK_FILE);
		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
			perror("Could not connect to socket");
      printf("%s\n",SERVER_SOCK_FILE);
			err = 2;
		}
	}
  // printf("Connnected: %s\n",SERVER_SOCK_FILE);
  if(err) closeSocketConnection();
  return err;
}

typedef void(*hook_fn)(char*);

// void setDisplayRemote(std::atomic<char[50]>& cur_display, std::atomic<bool>& mutex){
// void setDisplayRemote(char[50]& cur_display, std::atomic<bool>& mutex){
// void setDisplayRemote(char* & cur_display, std::atomic<bool>& mutex){
void setDisplayRemote(std::shared_ptr<char*>& cur_display, std::atomic<bool>& mutex){
	int len;
	char buff[8192];
	int err = 0;
  while (!mutex){
    if ((len = recv(fd, buff, 8192, 0)) < 0) {
      perror("recv");
      err = 4;
    }
    // printf ("receive %d %s\n", len, buff);
    if(!strncmp("focusedmon", buff, strlen("focusedmon"))){
      char* tok = strtok(buff,"\n");
      tok = strtok(tok,">>");
      tok = strtok(NULL,">>");
      tok = strtok(tok,",");
      strcpy(*cur_display.get(), tok);
      // printf ("%s",buff);
      // printf ("%s\n",tok);
    }
  }
}

/* void setDisplayReadHook(hook_fn hook){
  char cur_display[50] = {0};
  setDisplayRemote(cur_display);
  hook(cur_display);
}

  /* setDisplayReadHook([](char* cur_display) -> void {
    strcpy(cur_display, cur_mon);
  }); */

// }

#endif /* HYPRSOCKET_H */
