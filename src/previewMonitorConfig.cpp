#include <iostream>
// #include <json/>
#include <ncurses.h>
#include <signal.h>

#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <chrono>
using namespace std::chrono_literals;
#include <sstream>
// #include <algorithm>

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>

#include <cmath>

#include "runUnixCMD.h"
#include "hyprsocket.h"
#include "hyprsocket2.h"

enum rotation& operator++(enum rotation& r) {
    r = static_cast<enum rotation>((static_cast<int>(r) + 1) % (int)FLIPPED);
    return r;
}
enum rotation& operator--(enum rotation& r) {
    r = static_cast<enum rotation>((static_cast<int>(r) - 1) % (int)FLIPPED);
    return r;
}


cchar_t tr,tl,br,bl,s,h;

float maxfactor;

int row = LINES;
int col = COLS;

typedef struct WINDOW2 {
  struct monitor mon;
  WINDOW* win;
} WINDOW2;

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

std::mutex charLock;

// void hyprSocketFocusMonitor(std::atomic<char[50]> &cur_mon, std::atomic<bool> &mutex) {
// void hyprSocketFocusMonitor(char[50] &cur_mon, std::atomic<bool> &mutex) {
void hyprSocketFocusMonitor(char* cur_mon, std::atomic<bool> &mutex) {
// void hyprSocketFocusMonitor(std::shared_ptr<char[]> &cur_mon, std::atomic<bool> &mutex) {
  hs2_initSocketConnection();
  setDisplayRemote(cur_mon, mutex);
  hs2_closeSocketConnection();
}

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

struct monitor jsonMonitorToMonitor(const Json::Value& monitor) {
  return {
    .name = monitor["name"].asCString(),
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
    .focused = monitor["focused"].asBool()
  };
}

void createWindowsFromJson(std::vector<WINDOW2> &windows, const Json::Value &root) {
  for (const Json::Value &monitor : root){
    bool focused = monitor["focused"].asBool();
    if(focused){
      chg_border_col(4);
    }
    windows.push_back(createWindow(jsonMonitorToMonitor(monitor)));
    if(focused){
      wbkgd(windows.back().win, COLOR_PAIR(4));
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
    if(focused){
      chg_border_col(4);
    }
    for(auto& win2 : windows) {
      if(!strcmp(win2.mon.name, monitor["name"].asCString())){
        win2.mon = jsonMonitorToMonitor(monitor);
        updateWindow(win2.win, win2.mon);
      }
    }
    if(focused){
      wbkgd(windows.back().win, COLOR_PAIR(4));
      chg_border_col(0);
    }
  }
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

int main (int __attribute__((unused)) argc, char __attribute__((unused)) *argv[]) {

    mmask_t old = 0;

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
    std::vector<WINDOW2> windows;

    setcchar(&tl, L"╭", WA_NORMAL, 0, NULL);
    setcchar(&bl, L"╰", WA_NORMAL, 0, NULL);
    setcchar(&tr, L"╮", WA_NORMAL, 0, NULL);
    setcchar(&br, L"╯", WA_NORMAL, 0, NULL);
    setcchar(&s,  L"│", WA_NORMAL, 0, NULL);
    setcchar(&h,  L"─", WA_NORMAL, 0, NULL);

    if(!has_colors() || !can_change_color()){
      printw("Terminal doesnt support colors or cannot change them!");
      return -1;
    }
    start_color();
    use_default_colors(); //Set colors to normal
    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_BLACK, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    use_default_colors(); //Set colors to normal

    Json::Value root;
    if(!updateJson(root)){
      return 1;
    };

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

    // Thread
    std::atomic<bool> mutex(false);
    char cur_mon[50] = {0};
    std::thread t(hyprSocketFocusMonitor, cur_mon, std::ref(mutex));
    t.detach();

    // Get Max Size
    int maxwidth = 0;
    for (const auto &monitor : root){
      int tmpmaxwidth = monitor["width"].asInt() + monitor["x"].asInt();
      if (tmpmaxwidth > maxwidth)
        maxwidth = tmpmaxwidth;
    }
    int maxheight = 0;
    for (const auto &monitor : root){
      int tmpmaxheight = monitor["height"].asInt() + monitor["y"].asInt();
      if (tmpmaxheight > maxheight)
        maxheight = tmpmaxheight;
    }
    if (maxheight < maxwidth){
      maxfactor = (float)maxwidth;
    } else {
      maxfactor = (float)maxheight;
    }
    // End Get Max Size

    // First Render
    getmaxyx(stdscr, row, col);
    createWindowsFromJson(windows, root);
    renderWindows(windows, 250ms);
    // End First Render

    int ch = 0;
    char pre_mon[50];
    charLock.lock();
    std::strcpy(pre_mon, cur_mon);
    charLock.unlock();
    bool chg = false;
    // getch();
    nodelay(stdscr, TRUE);

    // goto error;

    // While
    WINDOW2* activeWin = nullptr;
    do {
      charLock.lock();
      if(std::strcmp(cur_mon, pre_mon)){
        std::strcpy(pre_mon, cur_mon);
        chg = true;
      }
      charLock.unlock();
      if(ch != ERR || chg) {
        // render
        if(ch == KEY_RESIZE || chg){
          if(!updateJson(root)) {
            goto error;
          }
          getmaxyx(stdscr, row, col);
          updateWindowsFromJson(windows, root);
          renderWindows(windows);
          /* mvprintw(1, 1, "status: %d", activeWin.mon.transform);
          std::this_thread::sleep_for(249ms); */
          chg = false;
        }
        else if(activeWin != nullptr && ch == 'r') {
          struct monitor &m = activeWin->mon;
          transformMonitor(m.name, ++m.transform);
          chg = true;
        }
        // mouse input
        else if (ch == KEY_MOUSE) {
          MEVENT event;
          if (getmouse(&event) == OK) {
            WINDOW2* win2 = getWindowAt(windows, event.x, event.y);
            WINDOW* win = win2 ? win2->win : nullptr;
            // if(event.bstate & REPORT_MOUSE_POSITION) {
              /* nodelay(stdscr, FALSE);
              ch = getch();
              nodelay(stdscr, TRUE);
              if(ch == 'r') {
                if(win2 != nullptr) {
                  refresh();
                  transformMonitor(m.name, ++m.transform);
                  wrefresh(win);
                  refresh();
                  chg = true;
                  continue;
                }
              } */
            // }
            if(event.bstate & BUTTON1_PRESSED){
              // mvprintw(1, 1, "Mouse event: %d at (%d,%d)", event.bstate, event.x, event.y);
              if(win2 != nullptr){
                refresh();
                chg_border_col(3);
                wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);
                wrefresh(win);
                refresh();
                activeWin = win2;
                chg = true;
              }
            } else if(event.bstate & BUTTON1_RELEASED) {
              refresh();
              for(auto& xwin2 : windows) {
                WINDOW* xwin = xwin2.win;
                  if(xwin2.mon.focused){
                    chg_border_col(4);
                  }
                  else{
                    chg_border_col(0);
                  }
                wborder_set(xwin, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);
                wrefresh(xwin);
              }
              // mvprintw(2, 1, "BUTTON1_RELEASED");
              chg_border_col(0);
              refresh();
              if(win2){
                struct monitor &m = win2->mon;
                transformMonitor(m.name, ++m.transform);
                chg = true;
              }
              activeWin = nullptr;
            }
          }
        }
        else if(ch == 'u') {
          chg = true;
        }
      }
    } while((ch = getch()) != 'q');

error:
    // Clean up and exit
    for(auto& win2 : windows){
      WINDOW* win = win2.win;
      delwin(win);
      win2.win = nullptr;
    }
    endwin();
    mutex = true;
    // hs_closeSocketConnection();
    std::this_thread::sleep_for(250ms);
    return 0;
}

