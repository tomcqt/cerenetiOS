#ifndef APP_H
#define APP_H

enum class AppAction {
  Continue, // app keeps running
  Exit,     // app wants to close (unused for now)
};

struct AppInterface {
  void (*init)();
  AppAction (*tick)();
  void (*shutdown)();
};

#endif