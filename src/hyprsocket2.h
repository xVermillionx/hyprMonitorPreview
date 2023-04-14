#ifndef HYPRSOCKET2_H
#define HYPRSOCKET2_H

// ns: hs2_

// extern "C" {

#include <stdio.h>  // printf
#include <sys/socket.h>
#include <unistd.h> // close
#include <sys/un.h> // sockaddr_un

#include "hyprglobal.h"

// cpp
#include <atomic>

static int hs2_fd;
static struct sockaddr_un hs2_addr;
static char hs2_SERVER_SOCK_FILE[100] = {0};

void hs2_closeSocketConnection() {
	if (hs2_fd >= 0) {
		close(hs2_fd);
    printf("Closed Socket: %d\n", hs2_fd);
	}
}

int hs2_initSocketConnection() {
	int err = 0;

  setHyprlandSocket(hs2_SERVER_SOCK_FILE, ".socket2.sock");

	if ((hs2_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create a socket");
		err = 1;
	}

	if (!err) {
		memset(&hs2_addr, 0, sizeof(hs2_addr));
		hs2_addr.sun_family = AF_UNIX;
		strcpy(hs2_addr.sun_path, hs2_SERVER_SOCK_FILE);
		if (connect(hs2_fd, (struct sockaddr *)&hs2_addr, sizeof(hs2_addr)) == -1) {
			perror("Could not connect to socket");
      printf("%s\n",hs2_SERVER_SOCK_FILE);
			err = 2;
		}
	}
  // printf("Connnected: %s\n",SERVER_SOCK_FILE);
  if(err) hs2_closeSocketConnection();
  return err;
}

typedef void(*hook_fn)(char*);

// void setDisplayRemote(std::atomic<char[50]>& cur_display, std::atomic<bool>& mutex){
// void setDisplayRemote(char[50]& cur_display, std::atomic<bool>& mutex){
#include <mutex>
extern std::mutex charLock;
void setDisplayRemote(char* cur_display, std::atomic<bool>& mutex){
// void setDisplayRemote(std::shared_ptr<char[]>& cur_display, std::atomic<bool>& mutex){
	int len;
	char buff[8000];
	int err = 0;
  while (!mutex){
    if ((len = recv(hs2_fd, buff, 8192, 0)) < 0) {
      perror("recv");
      err = 4;
    }
    // printf ("receive %d %s\n", len, buff);
    if(!strncmp("focusedmon", buff, strlen("focusedmon"))){
      char* tok = strtok(buff,"\n");
      tok = strtok(tok,">>");
      tok = strtok(NULL,">>");
      tok = strtok(tok,",");
      // strcpy(cur_display.get(), tok);
      charLock.lock();
      strcpy(cur_display, tok);
      charLock.unlock();
      // printf ("%s",buff);
      // printf ("%s\n",tok);
    }
    else if(!strncmp("monitorremoved", buff, strlen("monitorremoved"))){
      charLock.lock();
      strcpy(cur_display, strtok(buff,"\n"));
      charLock.unlock();
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

#endif /* HYPRSOCKET2_H */
