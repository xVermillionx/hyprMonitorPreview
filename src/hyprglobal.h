#ifndef HYPRGLOBAL_H
#define HYPRGLOBAL_H

#include <stdlib.h>
#include <string.h>

enum rotation {
  NORMAL = 0,
  DEG90,
  DEG180,
  DEG270,
  FLIPPED,
  FLIPPEDDEG90,
  FLIPPEDDEG180,
  FLIPPEDDEG270,
};

struct resolution {
  unsigned int x;
  unsigned int y;
};

struct position {
  unsigned int height;
  unsigned int width;
};

struct monitor {
  const char* name;
  struct position pos;
  struct resolution res;
  float hz;
  float scale;
  enum rotation transform;
  int id;
};

#define DEBUG_send(X) printf("%s\n", X)

static void setHyprlandSocket(char* sock_file, const char* end){
  strncpy((char*)sock_file, "/tmp/hypr/", 11);
  strncat((char*)sock_file, getenv("HYPRLAND_INSTANCE_SIGNATURE"), 70);
  strcat((char*)sock_file, "/");
  strncat((char*)sock_file, end, strlen(end));
}

#endif /* HYPRGLOBAL_H */
