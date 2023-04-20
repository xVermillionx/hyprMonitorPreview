#ifndef HYPRSOCKET_H
#define HYPRSOCKET_H

// ns: hs_

// extern "C" {

#include <stdio.h>  // printf
#include <sys/socket.h>
#include <unistd.h> // close
#include <sys/un.h> // sockaddr_un
#include <stdbool.h> // bool

#include "hyprglobal.h"

static int hs_fd;
static struct sockaddr_un hs_addr;
static char hs_SERVER_SOCK_FILE[100] = {0};
static char hs_CLIENT_SOCK_FILE[100] = "/tmp/hyprMonitorClientSocket";

void hs_closeSocketConnection() {
	if (hs_fd >= 0) {
    unlink(hs_CLIENT_SOCK_FILE);
		close(hs_fd);
    // printf("Closed Socket: %d\n", hs_fd);
	}
}

int hs_initSocketConnection() {
	int err = 0;

  setHyprlandSocket(hs_SERVER_SOCK_FILE, ".socket.sock");

	if ((hs_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Failed to create a socket");
		err = 1;
	}

  if (!err) {
		memset(&hs_addr, 0, sizeof(hs_addr));
		hs_addr.sun_family = AF_UNIX;
		strcpy(hs_addr.sun_path, hs_CLIENT_SOCK_FILE);
		unlink(hs_CLIENT_SOCK_FILE);
		if (bind(hs_fd, (struct sockaddr *)&hs_addr, sizeof(hs_addr)) < 0) {
			perror("Error on bind");
			err = 3;
		}
	}

	if (!err) {
		memset(&hs_addr, 0, sizeof(hs_addr));
		hs_addr.sun_family = AF_UNIX;
		strcpy(hs_addr.sun_path, hs_SERVER_SOCK_FILE);
		if (connect(hs_fd, (struct sockaddr *)&hs_addr, sizeof(hs_addr)) == -1) {
			perror("Could not connect to socket");
      printf("%s\n",hs_SERVER_SOCK_FILE);
			err = 2;
		}
	}
  // printf("Connnected: %s\n",SERVER_SOCK_FILE);
  if(err) hs_closeSocketConnection();
  return err;
}

typedef void(*hook_fn)(char*);

static const size_t hs_size = 8000;
char hs_buff[hs_size];

const char* sendCommandandRecieve(const char* command){
  hs_initSocketConnection();
	int len;
	int err = 0;
  // [flag(s)]/command args

  if (!err && send(hs_fd, command, strlen(command), 0) == -1) {
    perror("send");
    err = 5;
  }
  if (!err && (len = (int)recv(hs_fd, hs_buff, hs_size, 0)) < 0) {
    perror("recv");
    err = 4;
  }
  hs_closeSocketConnection();
  return hs_buff;
}

const char* getMonitors() {
  return sendCommandandRecieve("[j]/monitors");
}

bool transformMonitor(const char* monitor, enum rotation rot) {
  char cmd[100];
  if(snprintf(cmd, 100, "[]/keyword monitor %s,transform,%d", monitor, rot) < 0){
    return false;
  };
  return !strncmp(sendCommandandRecieve(cmd), "ok", 2);
}

bool mirrorMonitor(const char* monitor, const char* monitor_to_mirror) {
  char cmd[100];
  if(snprintf(cmd, 100, "[]/keyword monitor %s,mirror,%s", monitor, monitor_to_mirror) < 0){
    return false;
  };
  return !strncmp(sendCommandandRecieve(cmd), "ok", 2);
}

#define ON  true
#define OFF false
bool setDPMSMonitor(const char* monitor, bool enable) {
  char cmd[100];
  if(snprintf(cmd, 100, "[]/dispatch dpms %s,on %s", enable ? "on" : "off", monitor) < 0){
    return false;
  };
  return !strncmp(sendCommandandRecieve(cmd), "ok", 2);
}

bool setStatusMonitor(const char* monitor, bool enable) {
  char cmd[100];
  if(snprintf(cmd, 100, "[]/keyword monitor %s,disable,%d", monitor, enable) < 0){
    return false;
  };
  return !strncmp(sendCommandandRecieve(cmd), "ok", 2);
}

bool setMonitor(struct monitor m) {
  char cmd[200];
  //ERROR: Check why/how pos and res have to be switched...
  // if(snprintf(cmd, 200, "[]/keyword monitor %s,%dx%d@%.2f,%dx%d,%.2f,transform,%d", m.name, m.res.x, m.res.y, m.hz, m.pos.width, m.pos.height, m.scale, m.transform) < 0){
  if(snprintf(cmd, 200, "[]/keyword monitor %s,%dx%d@%.2f,%dx%d,%.2f,transform,%d", m.name, m.pos.width, m.pos.height, m.hz, m.res.x, m.res.y, m.scale, m.transform) < 0){
    return false;
  };
  return !strncmp(sendCommandandRecieve(cmd), "ok", 2);
}

#endif /* HYPRSOCKET_H */
