#ifndef HELPER_H
#define HELPER_H

#include <ncurses.h>
#include <cmath>
#include <signal.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "hyprglobal.h"

float maxfactor;

int row = LINES;
int col = COLS;

cchar_t tr,tl,br,bl,s,h;

typedef struct WINDOW2 {
  struct monitor mon;
  WINDOW* win;
  bool operator==(const WINDOW2& c) const {
      return (this == &c);
  }
} WINDOW2;

void start_curses(mmask_t old){
  initscr();       // Initialize ncurses screen
  cbreak();
  noecho();        // Don't display user input
  keypad(stdscr, TRUE);
  // Enable mouse support and track all mouse events
  // mousemask(BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON3_CLICKED | BUTTON4_PRESSED | BUTTON5_PRESSED | REPORT_MOUSE_POSITION | ALL_MOUSE_EVENTS, &old);
  mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON3_CLICKED |  REPORT_MOUSE_POSITION | ALL_MOUSE_EVENTS, &old);
  // Set the mouse click interval to 1000 milliseconds (1 second)
  mouseinterval(80);
  curs_set(0);     // Hide cursor

  setlocale(LC_ALL, "");

  setcchar(&tl, L"╭", WA_NORMAL, 0, NULL);
  setcchar(&bl, L"╰", WA_NORMAL, 0, NULL);
  setcchar(&tr, L"╮", WA_NORMAL, 0, NULL);
  setcchar(&br, L"╯", WA_NORMAL, 0, NULL);
  setcchar(&s,  L"│", WA_NORMAL, 0, NULL);
  setcchar(&h,  L"─", WA_NORMAL, 0, NULL);

  if(!has_colors() || !can_change_color()){
    printw("Terminal doesnt support colors or cannot change them!");
    exit(-1);
  }
  start_color();
  use_default_colors(); //Set colors to normal
  init_pair(1, COLOR_WHITE, -1);
  init_pair(2, COLOR_BLACK, -1);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_BLUE, -1);
  use_default_colors(); //Set colors to normal

  signal(SIGWINCH, [](int __attribute__((unused)) sig)->void {
    endwin(); // End curses mode

    refresh(); // Clear the screen
    // Handle the resize event here
    // ...
    refresh(); // Redraw the screen
    // Re-initialize ncurses
    initscr();
    // Turn off keyboard echoing and enable special key input
    noecho();
    keypad(stdscr, TRUE);
  });
}

enum rotation& operator++(enum rotation& r) {
    r = static_cast<enum rotation>((static_cast<int>(r) + 1) % (int)FLIPPED);
    return r;
}
enum rotation& operator--(enum rotation& r) {
    r = static_cast<enum rotation>((static_cast<int>(r) - 1) % (int)FLIPPED);
    return r;
}

void updateMonitorFromWindow(WINDOW* win, struct monitor& m){
  int height, width;
  getmaxyx(win, height, width);
  int x, y;
  getbegyx(win, x, y);

  float hfactor = maxfactor;
  float wfactor = hfactor/2;

  if(m.transform%2){
    m.pos.height = width  * hfactor * m.scale / (float)col;
    m.pos.width  = height * wfactor * m.scale / (float)row;
  } else {
    m.pos.height = height * wfactor * m.scale / (float)row;
    m.pos.width  = width  * hfactor * m.scale / (float)col;
  }
  m.res.y = y * wfactor / row;
  m.res.x = x * hfactor / col;
}

void updateWindow(WINDOW* win, struct monitor m) {
  int height, width;

  float hfactor = maxfactor;
  float wfactor = hfactor/2;

  if(m.transform%2){
    height = floor(row * (m.pos.width /wfactor/m.scale));
    width  = floor(col * (m.pos.height/hfactor/m.scale));
  } else {
    height = floor(row * (m.pos.height/wfactor/m.scale));
    width  = floor(col * (m.pos.width /hfactor/m.scale));
  }
  int y    = floor(row * (m.res.y/wfactor));
  int x    = floor(col * (m.res.x/hfactor));

  // Order matters!
  wresize(win, height, width);
  mvwin(win, y, x);
  wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);

  int hcenter = height/2;
  int wcenter = width/2;
  mvwprintw(win, hcenter, wcenter-strlen(m.name)/2, m.name);
  std::stringstream s;
  s << x << ' ' << m.pos.width << 'x' << m.pos.height;
  std::string dim = s.str();
  mvwprintw(win, hcenter+1, wcenter-dim.length()/2, dim.c_str());
}

WINDOW2 createWindow(struct monitor m){
    WINDOW *win = newwin(0, 0, 0, 0);
    box_set(win, 0, 0); // Create a box around the window
    updateWindow(win, m);
    WINDOW2 ret{m, win};
    return ret;
}

void chg_cchar_col(cchar_t &c, NCURSES_PAIRS_T col_pair){
  c.ext_color = col_pair;
}


// set borders color 0 for normal
void chg_border_col(NCURSES_PAIRS_T col_pair){
  chg_cchar_col(tr, col_pair);
  chg_cchar_col(tl, col_pair);
  chg_cchar_col(br, col_pair);
  chg_cchar_col(bl, col_pair);
  chg_cchar_col(s,  col_pair);
  chg_cchar_col(h,  col_pair);
}
// wcolor_set(windows.back().win,COLOR_PAIR(5),nullptr); ??

bool updateJson(Json::Value& root){
  root.clear();
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(getMonitors(), root);
  if (!parsingSuccessful)
  {
    std::cout << "Failed to parse"
              << reader.getFormattedErrorMessages();
  }
  return parsingSuccessful;
}

void jsonMonitorToMonitorUpdate(struct monitor &m, const Json::Value& monitor) {
    // m.name       = monitor["name"].asCString();
  /* if(m.name)
    delete m.name;
  m.name = new char[monitor["name"].asString().length()];
  strcpy(m.name, monitor["name"].asCString()); */
    m.pos.height = monitor["height"].asUInt();
    m.pos.width  = monitor["width"].asUInt();
    m.res.x      = monitor["x"].asUInt();
    m.res.y      = monitor["y"].asUInt();
    m.hz         = monitor["refreshRate"].asFloat();
    m.scale      = monitor["scale"].asFloat();
    m.transform  = (enum rotation)monitor["transform"].asUInt();
    m.id         = monitor["id"].asInt();
    m.focused    = monitor["focused"].asBool();
    m.dpms       = monitor["dpmsStatus"].asBool();
}

struct monitor jsonMonitorToMonitor(const Json::Value& monitor) {
  char* name = new char[monitor["name"].asString().length()];
  strcpy(name, monitor["name"].asCString());

  return {
    // .name = monitor["name"].asCString(),
    .name = name,
    .pos = {
      .height = monitor["height"].asUInt(),
      .width = monitor["width"].asUInt()
    },
    .res = {
      .x = monitor["x"].asUInt(),
      .y = monitor["y"].asUInt()
    },
    .hz = monitor["refreshRate"].asFloat(),
    .scale = monitor["scale"].asFloat(),
    .transform = (enum rotation)monitor["transform"].asUInt(),
    .id = monitor["id"].asInt(),
    .focused = monitor["focused"].asBool(),
    .dpms = monitor["dpmsStatus"].asBool()
  };
}

WINDOW2* getWindowAt(std::vector<WINDOW2> &windows, const char* name){
  /* WINDOW2* ret = nullptr;
  for(auto& win2 : windows){
    struct monitor& m = win2.mon;
    if(!strcmp(name, m.name)) {
      ret = &win2;
      break;
    }
  }
  return ret; */
  for(auto& win2 : windows){
    struct monitor& m = win2.mon;
    if(!strcmp(name, m.name)) {
      return &win2;
    }
  }
  return nullptr;
}

WINDOW2* getWindowAt(std::vector<WINDOW2> &windows, int x, int y){
  WINDOW2* ret = nullptr;
  for(auto& win2 : windows){
    WINDOW* win = win2.win;
    if(wenclose(win, y, x)) {
      ret = &win2;
      break;
    }
  }
  return ret;
}

void renderWindows(std::vector<WINDOW2> &windows, std::chrono::milliseconds delay = 0ms) {
  refresh();
  for(const auto& win2 : windows) {
    WINDOW* win = win2.win;
    wrefresh(win);
    if(delay > 0ms) std::this_thread::sleep_for(delay);
  }
  refresh();
}

void createWindowsFromJson(std::vector<WINDOW2> &windows, const Json::Value &root) {
  for (const Json::Value &monitor : root){
    bool focused = monitor["focused"].asBool();
    if(focused){
      chg_border_col(4);
    }
    windows.push_back(createWindow(jsonMonitorToMonitor(monitor)));
    WINDOW2& win2 = windows.back();
    // setStatusMonitor(win2.mon, true);
    if(focused){
      wbkgd(win2.win, COLOR_PAIR(4));
      chg_border_col(0);
    }
  }
}

void updateWindowsFromOwnMonitor(std::vector<WINDOW2> &windows){
  for(auto& win2 : windows) {
    updateWindow(win2.win, win2.mon);
  }
}

void updateWindowsFromJson(std::vector<WINDOW2> &windows, const Json::Value &root) {
  for (const auto &monitor : root){
    bool focused = monitor["focused"].asBool();
    /* if(focused){
      chg_border_col(4);
    } */
      WINDOW2* win2 = getWindowAt(windows, monitor["name"].asCString());
      // win2.mon = jsonMonitorToMonitor(monitor);
      if(win2){
        jsonMonitorToMonitorUpdate(win2->mon, monitor);
        werase(win2->win);   // Fill the old monitor model with blanks
        wrefresh(win2->win); // Clear the old monitor model
        updateWindow(win2->win, win2->mon);
        // wrefresh(win2->win); // redraw new monitor model
      } else { // Create Monitor if it doesnt exist
        windows.push_back(createWindow(jsonMonitorToMonitor(monitor)));
        wrefresh(windows.back().win); // Draw new monitor model
        win2 = &(windows.back());
      }
    if(focused){
      wbkgd(win2->win, COLOR_PAIR(4));
      // chg_border_col(0);
    } else {
      wbkgd(win2->win, COLOR_PAIR(0));
    }
  }
}


#endif /* HELPER_H */
