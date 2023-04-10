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
#include <sstream>
// #include <algorithm>

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>

#include <cmath>

#include "runUnixCMD.h"
#include "hyprsocket.h"

enum rotation {
  NORMAL = 0,
  DEG90,
  DEG180,
  DEG270,
};

struct monitor {
  const char* name;
  int height;
  int width;
  int x;
  int y;
  float scale;
  enum rotation transform;
  int id;
  void print(){
    std::cout << name << " " << width << "x" << height << "@" << x << "," << y << " " << scale << " " << transform << std::endl;
  };
};

cchar_t tr,tl,br,bl,s,h;

float maxfactor;

int row = LINES;
int col = COLS;

WINDOW* createWindow(struct monitor m){
  int height, width;
  // float hfactor = 7000.0f;
  float hfactor = maxfactor;
  float wfactor = hfactor/2;

  if(m.transform%2){
    height = floor(row * (m.width/wfactor/m.scale));
    width  = floor(col  * (m.height/hfactor/m.scale));
  } else {
    height = floor(row * (m.height/wfactor/m.scale));
    width  = floor(col  * (m.width/hfactor/m.scale));
  }
  int y    = floor(row * (m.y/wfactor));
  int x    = floor(col  * (m.x/hfactor));

    WINDOW *win = newwin(height, width, y, x);
    // wattron(win, COLOR_PAIR(10));
    box_set(win, 0, 0); // Create a box around the window
    wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);
    // wattroff(win, COLOR_PAIR(10));

    int hcenter = height/2;
    int wcenter = width/2;
    mvwprintw(win,hcenter,wcenter-strlen(m.name)/2,m.name);
    std::stringstream s;
    s << x << ' ' << m.width << 'x' << m.height;
    std::string dim = s.str();
    mvwprintw(win,hcenter+1,wcenter-dim.length()/2,dim.c_str());
    // mvwprintw(win,1,1,m.name);

    return win;
}

void chg_cchar_col(cchar_t &c, NCURSES_PAIRS_T col_pair){
  c.ext_color = col_pair;
}

void chg_border_col(NCURSES_PAIRS_T col_pair){
  chg_cchar_col(tr, col_pair);
  chg_cchar_col(tl, col_pair);
  chg_cchar_col(br, col_pair);
  chg_cchar_col(bl, col_pair);
  chg_cchar_col(s,  col_pair);
  chg_cchar_col(h,  col_pair);
}

std::mutex charLock;

// void hyprSocketFocusMonitor(std::atomic<char[50]> &cur_mon, std::atomic<bool> &mutex) {
// void hyprSocketFocusMonitor(char[50] &cur_mon, std::atomic<bool> &mutex) {
void hyprSocketFocusMonitor(char* cur_mon, std::atomic<bool> &mutex) {
// void hyprSocketFocusMonitor(std::shared_ptr<char[]> &cur_mon, std::atomic<bool> &mutex) {
  initSocketConnection();
  setDisplayRemote(cur_mon, mutex);
  closeSocketConnection();
}

int main (int argc, char* argv[]) {

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
    std::vector<WINDOW*> windows;

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

    /* attron(COLOR_PAIR(5));
    printw("Test\n");
    attroff(COLOR_PAIR(5));

    refresh(); // Clear the screen */
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    Json::Value root;
    Json::Reader reader;
    std::string hyprmon = runUnixCommandAndCaptureOutput("hyprctl monitors -j");
    bool parsingSuccessful = reader.parse(hyprmon.c_str(), root);
    if (!parsingSuccessful)
    {
      std::cout << "Failed to parse"
                << reader.getFormattedErrorMessages();
      return 0;
    }

    signal(SIGWINCH, [](int sig)->void {
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

    std::atomic<bool> mutex(false);
    // std::shared_ptr<char[]> cur_mon = std::make_shared<char[]>(50); // cpp20
    char cur_mon[50] = {0};
    std::thread t(hyprSocketFocusMonitor, cur_mon, std::ref(mutex));
    t.detach();

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
      maxfactor = maxwidth;
    } else {
      maxfactor = maxheight;
    }

    // Wait for user input
    int ch = 0;
    char pre_mon[50];
    // std::strcpy(pre_mon, cur_mon.get());
    charLock.lock();
    std::strcpy(pre_mon, cur_mon);
    charLock.unlock();
    bool chg = false;
    nodelay(stdscr, TRUE);
    do {
      charLock.lock();
      /* if(std::strcmp(cur_mon.get(), pre_mon)){
        std::strcpy(pre_mon, cur_mon.get());
        chg = true;
      } */
      if(std::strcmp(cur_mon, pre_mon)){
        std::strcpy(pre_mon, cur_mon);
        chg = true;
      }
      charLock.unlock();
      if(ch != ERR || chg){
        if(ch == 0 || ch == KEY_RESIZE || chg){
          getmaxyx(stdscr, row, col);
          for (const auto &monitor : root){
            bool focused;
            if(!*pre_mon){
              focused = monitor["focused"].asBool();
            } else {
              focused = !std::strcmp(monitor["name"].asCString(), (char*)pre_mon);
            }
            // monitor["focused"] = focused;

            if(focused){
              chg_border_col(4);
            }
            struct monitor m = {
              // cur_mon.get(),
              // pre_mon,
              monitor["name"].asCString(),
              monitor["height"].asInt(),
              monitor["width"].asInt(),
              monitor["x"].asInt(),
              monitor["y"].asInt(),
              monitor["scale"].asFloat(),
              (enum rotation)monitor["transform"].asInt(),
              monitor["id"].asInt()
            };
            // m.print();
            windows.push_back(createWindow(m));
            if(focused){
              wbkgd(windows.back(),COLOR_PAIR(4)); // Set Color of window
              chg_border_col(0); // set borders to normal
              // wcolor_set(windows.back(),COLOR_PAIR(5),nullptr); ??
            }
          }

          refresh();
          for(auto win : windows){
            wrefresh(win);
            if (ch == 0)
              std::this_thread::sleep_for(std::chrono::milliseconds(250));
          }
          chg = false;
        } else if (ch == KEY_MOUSE) {
          MEVENT event;
          if (getmouse(&event) == OK) {
            if(event.bstate & BUTTON1_PRESSED){
              for(auto win : windows){
                refresh();
                if(wenclose(win, event.y, event.x)){
                  chg_border_col(3);
                } else {
                  chg_border_col(0);
                }
                wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);
                wrefresh(win);
                // mvprintw(1, 1, "Mouse event: %d at (%d,%d)", event.bstate, event.x, event.y);
              }
            // mvprintw(2, 1, "BUTTON1_PRESSED");
            } else if(event.bstate & BUTTON1_RELEASED){
              for(auto& win : windows){
                chg_border_col(0);
                wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);
                wrefresh(win);
              }
            // mvprintw(2, 1, "BUTTON1_RELEASED");
            }
            refresh();
          }
        }
      }
    } while((ch = getch()) != 'q');

    // Clean up and exit
    for(auto win : windows){
      delwin(win);
    }
    endwin();
    mutex = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    return 0;
}

