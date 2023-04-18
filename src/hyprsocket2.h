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
#ifdef DEBUG
    printf("Closed Socket: %d\n", hs2_fd);
#endif
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

#include <mutex>
extern std::mutex charLock;
void setDisplayRemote(char* cur_display, std::atomic<bool>& mutex){
	int len;
  const size_t size = 8000;
	char buff[size];
	int err = 0;
  while (!mutex){
    if ((len = (int)recv(hs2_fd, buff, size, 0)) < 0) {
      perror("recv");
      err = 4;
    }
    if(!err){
      if(!strncmp("focusedmon", buff, strlen("focusedmon"))){
        char* tok = strtok(buff,"\n");
        tok = strtok(tok,">>");
        tok = strtok(NULL,">>");
        tok = strtok(tok,",");
        charLock.lock();
        strcpy(cur_display, tok);
        charLock.unlock();
      }
      else if(!strncmp("monitorremoved", buff, strlen("monitorremoved"))){
        charLock.lock();
        strcpy(cur_display, strtok(buff,"\n"));
        charLock.unlock();
      }
    }
  }
}

#endif /* HYPRSOCKET2_H */
