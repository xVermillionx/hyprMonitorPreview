#include <iostream>
// #include <json/>
#include <ncurses.h>
#include <signal.h>

#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
// #include <algorithm>

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>

#include <cmath>

#include "runUnixCMD.h"

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
    box_set(win, 0, 0); // Create a box around the window
    wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);

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
    mouseinterval(250);

    curs_set(0);     // Hide cursor


    setlocale(LC_ALL, "");
    std::vector<WINDOW*> windows;

    setcchar(&tl, L"╭", WA_NORMAL, 4, NULL);
    setcchar(&bl, L"╰", WA_NORMAL, 4, NULL);
    setcchar(&tr, L"╮", WA_NORMAL, 4, NULL);
    setcchar(&br, L"╯", WA_NORMAL, 4, NULL);
    setcchar(&s,  L"│", WA_NORMAL, 4, NULL);
    setcchar(&h,  L"─", WA_NORMAL, 4, NULL);

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
    while(1) {
      if(ch == 0 || ch == KEY_RESIZE){
        getmaxyx(stdscr, row, col);
        for (const auto &monitor : root){
          struct monitor m = {
            monitor["name"].asString().c_str(),
            monitor["height"].asInt(),
            monitor["width"].asInt(),
            monitor["x"].asInt(),
            monitor["y"].asInt(),
            monitor["scale"].asFloat(),
            (enum rotation)monitor["transform"].asInt(),
            monitor["id"].asInt()
          };
          m.print();
          windows.push_back(createWindow(m));
        }

        refresh();
        for(auto win : windows){
          wrefresh(win);
          if (ch == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
      } else if (ch == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK) {
          // mvprintw(1, 1, "Mouse event: %d at (%d,%d)", event.bstate, event.x, event.y);
          if(event.bstate & BUTTON1_PRESSED){
          // mvprintw(2, 1, "BUTTON1_PRESSED");
          } else if(event.bstate & BUTTON1_RELEASED){
          // mvprintw(2, 1, "BUTTON1_RELEASED");
          }
          refresh();
        }
      } else {
        break;
      }
      ch = getch();
    }

    // Clean up and exit
    for(auto win : windows){
      delwin(win);
    }
    endwin();
    return 0;
}

