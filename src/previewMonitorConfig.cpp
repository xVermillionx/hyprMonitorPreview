#include <iostream>
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

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>

#include <cmath>

#include "runUnixCMD.h"
#include "hyprsocket.h"
#include "hyprsocket2.h"
#include "helpers.h"

std::mutex charLock;

void hyprSocketFocusMonitor(char* cur_mon, std::atomic<bool> &mutex) {
  hs2_initSocketConnection();
  setDisplayRemote(cur_mon, mutex);
  hs2_closeSocketConnection();
}

int main (int __attribute__((unused)) argc, char __attribute__((unused)) *argv[]) {
    mmask_t old = 0;
    start_curses(old);

    // Thread for events
    std::atomic<bool> mutex(false);
    char cur_mon[50] = {0};
    std::thread t(hyprSocketFocusMonitor, cur_mon, std::ref(mutex));
    t.detach();

    std::vector<WINDOW2> windows;

    Json::Value root;
    if(!updateJson(root)){
      return 1;
    };

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
          getmaxyx(stdscr, row, col);
          if(!updateJson(root)) {
            goto error;
          }
          updateWindowsFromJson(windows, root);
          renderWindows(windows);
          if(chg) std::this_thread::sleep_for(250ms);
          chg = false;
        }
        // mouse input
        else if (ch == KEY_MOUSE) {
          MEVENT event;
          if (getmouse(&event) == OK) {
            WINDOW2* win2 = getWindowAt(windows, event.x, event.y);
            WINDOW* win = win2 ? win2->win : nullptr;
            // mvprintw(1, 1, "Mouse event: %d at (%d,%d)", event.bstate, event.x, event.y);
            /* if(event.bstate & REPORT_MOUSE_POSITION) {
            } */
            if(event.bstate & BUTTON1_PRESSED){
              if(win2 != nullptr){
                refresh();
                chg_border_col(3);
                wborder_set(win, (const cchar_t*)&s, (const cchar_t*)&s, (const cchar_t*)&h, (const cchar_t*)&h, (const cchar_t*)&tl, (const cchar_t*)&tr, (const cchar_t*)&bl, (const cchar_t*)&br);
                wrefresh(win);
                refresh();
                activeWin = win2;
                // chg = true;
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
              chg_border_col(0);
              refresh();
              if(win2) {
                struct monitor &m = win2->mon;
                // FIX: not rotatting if chg is set...
                chg = transformMonitor(m.name, ++m.transform);
                // transformMonitor(m.name, ++m.transform);
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

// Clean up and exit
error:
    for(auto& win2 : windows){
      WINDOW* win = win2.win;
      delwin(win);
      win2.win = nullptr;
    }
    endwin();
    // Stop detached thread
    mutex = true;
    std::this_thread::sleep_for(250ms);
    return 0;
}

