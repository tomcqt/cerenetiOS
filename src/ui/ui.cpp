#include "ui/ui.h"
#include "gfx/gfx.h"
#include "hal/input.h"
#include "gfx/colors.h"
#include "images/icons.h"
#include "hal/power.h"

#include <graphx.h>
#include <sys/rtc.h>
#include <fontlibc.h>

#include <cstdio>
#include <cstring>
#include <cstdint>

namespace ui {
  // --- layout constants ---

  // status bar
  static constexpr int STATUS_H = 14;

  // dock
  static constexpr int DOCK_H         = 38;
  static constexpr int DOCK_Y         = GFX_LCD_HEIGHT - DOCK_H;
  static constexpr int DOCK_MARGIN    = 4;
  static constexpr int DOCK_ICON_SIZE = 28;
  static constexpr int DOCK_PADDING   = 6;
  static constexpr int DOCK_RADIUS    = 6;

  // launcher grid
  static constexpr int GRID_COLS      = 5;
  static constexpr int GRID_ROWS      = 4;
  static constexpr int GRID_ICON_SIZE = 28;
  static constexpr int GRID_SPACING_X = 24;
  static constexpr int GRID_SPACING_Y = 20;
  static constexpr int GRID_LABEL_H   = 10;
  static constexpr int GRID_TOP       = STATUS_H + 10;

  // --- app definitions ---

  struct App {
    const char*         name;
    uint8_t             icon_color;
    const gfx_sprite_t *icon_glyph;  // single char for placeholder icons 
  };

  static const App all_apps[] = {
    { "Calc",     CLR_RED,     calc },
    { "Graph",    CLR_LIME,    graph },
    { "Programs", CLR_CYAN,    programs },
    { "Files",    CLR_L_PINK,  files },
    { "Notes",    CLR_YELLOW,  notes },
    { "Stats",    CLR_D_RED,   stats },
    { "Table",    CLR_D_GREEN, table },
    { "Settings", CLR_L_GRAY,  settings },
    { "Clock",    CLR_L_CYAN,  clock },
    { "Memory",   CLR_PURPLE,  memory },
    { "About",    CLR_GRAY,    about }
  };

  static constexpr int APP_COUNT = sizeof(all_apps) / sizeof(App);

  // pinned dock apps (indexes)
  static const int pinned[] = { 0, 1, 2, 3, 8 };
  static constexpr int PINNED_COUNT = sizeof(pinned) / sizeof(pinned[0]);
  // last slot is the launcher
  static constexpr int DOCK_SLOTS = PINNED_COUNT + 1;

  // --- state ---

  enum class Screen {
    Home,
    Launcher,
  };

  static Screen current_screen = Screen::Home;
  static int    dock_sel       = 0;
  static int    grid_sel       = 0;
  static int    grid_page      = 0;

  static constexpr int GRID_PER_PAGE = GRID_COLS * GRID_ROWS;
  static int grid_page_count() {
    return (APP_COUNT + GRID_PER_PAGE - 1) / GRID_PER_PAGE;
  }

  static int selected_app = -1; // -1 = no selection
  static bool was_dismissed = false;

  // --- drawing helpers ---

  enum Quadrant {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
  };

  // draw filled quarter circle (for rounded rectangles)
  static void fill_quarter_circle(int cx, int cy, int r, Quadrant quadrant) {
    for (int y = -r; y <= r; y++) {
      for (int x = -r; x <= r; x++) {
        if (x * x + y * y <= r * r) {
          switch (quadrant) {
            case Quadrant::TopLeft:     if (x <= 0 && y <= 0) gfx_SetPixel(cx + x, cy + y); break;
            case Quadrant::TopRight:    if (x >= 0 && y <= 0) gfx_SetPixel(cx + x, cy + y); break;
            case Quadrant::BottomLeft:  if (x <= 0 && y >= 0) gfx_SetPixel(cx + x, cy + y); break;
            case Quadrant::BottomRight: if (x >= 0 && y >= 0) gfx_SetPixel(cx + x, cy + y); break;
          }
        }
      }
    }
  }

  // draw a rounded-looking rectangle (aproximated with filled_rect & edge pixels)
  static void draw_dock_bg() {
    int dock_content_w = DOCK_SLOTS * (DOCK_ICON_SIZE + DOCK_PADDING) + DOCK_PADDING;
    int dock_x = (GFX_LCD_WIDTH - dock_content_w) / 2;
    int r = DOCK_RADIUS;

    // shadow
    gfx_SetColor(gfx::Color::DarkGray);

    gfx_FillRectangle(dock_x + 1 + r, DOCK_Y + 1, dock_content_w - 2*r, DOCK_H - 2);
    gfx_FillRectangle(dock_x + 1, DOCK_Y + 1 + r, dock_content_w, DOCK_H - 2 - 2*r);

    fill_quarter_circle(dock_x + 1 + r, DOCK_Y + 1 + r, r, Quadrant::TopLeft);
    fill_quarter_circle(dock_x + dock_content_w - r, DOCK_Y + 1 + r, r, Quadrant::TopRight);  
    fill_quarter_circle(dock_x + 1 + r, DOCK_Y + DOCK_H - r - 1, r, Quadrant::BottomLeft);
    fill_quarter_circle(dock_x + dock_content_w - r, DOCK_Y + DOCK_H - r - 1, r, Quadrant::BottomRight);

    // main dock
    gfx_SetColor(CLR_D_GRAY);

    gfx_FillRectangle(dock_x + r, DOCK_Y, dock_content_w - 2*r, DOCK_H - 2);
    gfx_FillRectangle(dock_x, DOCK_Y + r, dock_content_w, DOCK_H - 2 - 2*r);

    fill_quarter_circle(dock_x + r, DOCK_Y + r, r, Quadrant::TopLeft);
    fill_quarter_circle(dock_x + dock_content_w - r, DOCK_Y + r, r, Quadrant::TopRight);
    fill_quarter_circle(dock_x + r, DOCK_Y + DOCK_H - r - 2, r, Quadrant::BottomLeft);
    fill_quarter_circle(dock_x + dock_content_w - r, DOCK_Y + DOCK_H - r - 2, r, Quadrant::BottomRight);

    // highlight line (trimmed for radius)
    gfx_SetColor(CLR_P_PURPLE);
    gfx_HorizLine(dock_x + r, DOCK_Y, dock_content_w - 2*r);
  }

  static void draw_icon(int cx, int cy, int size, uint8_t color, const gfx_sprite_t *icon, bool selected) {
    int x = cx - size / 2;
    int y = cy - size / 2;

    if (selected) {
      // selection highlight behind icon
      gfx_SetColor(gfx::Color::White);
      gfx_FillRectangle(x - 2, y - 2, size + 4, size + 4);
    }

    // icon background
    gfx_SetColor(color);
    gfx_FillRectangle(x, y, size, size);

    // border
    gfx_SetColor(gfx::Color::Black);
    gfx_Rectangle(x, y, size, size);

    // glyph
    // char buf[2] = { glyph, '\0' };
    // gfx_SetTextFGColor(gfx::Color::White);
    // gfx_SetTextBGColor(color);
    // gfx_SetTextTransparentColor(color);
    // gfx_PrintStringXY(buf, cx - 3, cy - 4);
    gfx_TransparentSprite(icon, x + 2, y + 2);
  }

  // --- status bar ---

  static void draw_status_bar() {
    // background
    gfx::draw_rect_filled(0, 0, GFX_LCD_WIDTH, STATUS_H, gfx::Color::DarkGray);

    // os name
    gfx::draw_text("cerenetiOS", 4, 2, gfx::Color::White);

    // time
    char time_buf[16];
    uint8_t h = rtc_Hours;
    uint8_t m = rtc_Minutes;
    snprintf(time_buf, sizeof(time_buf), "%02u:%02u", (unsigned)h, (unsigned)m);
    gfx::draw_text(time_buf, GFX_LCD_WIDTH / 2 - 12, 2, gfx::Color::White);

    // battery
    char battery_value[4];
    snprintf(battery_value, sizeof(battery_value), "%d", hal::Power::percent());
    strcat(battery_value, "%");
    gfx::draw_text(battery_value, GFX_LCD_WIDTH - 28, 2, gfx::Color::LightGray);
  }

  // --- dock ---

  static int dock_icon_x(int slot) {
    int dock_content_w = DOCK_SLOTS * (DOCK_ICON_SIZE + DOCK_PADDING) + DOCK_PADDING;
    int dock_x = (GFX_LCD_WIDTH - dock_content_w) / 2;
    return dock_x + DOCK_PADDING + slot * (DOCK_ICON_SIZE + DOCK_PADDING) + DOCK_ICON_SIZE / 2;
  }

  static int dock_icon_y() {
    return DOCK_Y + DOCK_MARGIN + DOCK_ICON_SIZE / 2;
  }

  static void draw_dock() {
    draw_dock_bg();

    int cy = dock_icon_y();
    const char* hovered_name = nullptr;

    // pinned app icons
    for (int i = 0; i < PINNED_COUNT; i++) {
      const App& app = all_apps[pinned[i]];
      bool sel = (current_screen == Screen::Home && dock_sel == i);
      draw_icon(dock_icon_x(i), cy, DOCK_ICON_SIZE, app.icon_color, app.icon_glyph, sel);
      if (sel) hovered_name = app.name;
    }

    // launcher icon (last slot)
    {
      int slot = PINNED_COUNT;
      bool sel = (current_screen == Screen::Home && dock_sel == slot);
      int cx = dock_icon_x(slot);

      if (sel) {
        gfx_SetColor(gfx::Color::White);
        gfx_FillRectangle(cx - DOCK_ICON_SIZE / 2 - 2, cy - DOCK_ICON_SIZE / 2 - 2, DOCK_ICON_SIZE + 4, DOCK_ICON_SIZE + 4);
        hovered_name = "Launcher";
      }

      // draw 3x3 grid for launcher icon
      gfx_SetColor(gfx::Color::LightGray);
      gfx_FillRectangle(cx - DOCK_ICON_SIZE / 2, cy - DOCK_ICON_SIZE / 2, DOCK_ICON_SIZE, DOCK_ICON_SIZE);
      gfx_SetColor(gfx::Color::Black);
      gfx_Rectangle(cx - DOCK_ICON_SIZE / 2, cy - DOCK_ICON_SIZE / 2, DOCK_ICON_SIZE, DOCK_ICON_SIZE);

      for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
          int dx = cx - 8 + c * 8;
          int dy = cy - 8 + r * 8;
          gfx_SetColor(gfx::Color::White);
          gfx_FillRectangle(dx, dy, 4, 4);
        }
      }
    }

    // tooltip for selected app
    if (hovered_name != nullptr) {
      int label_w = fontlib_GetStringWidth(hovered_name) + 4;
      int label_x = (GFX_LCD_WIDTH - label_w) / 2;
      int label_y = DOCK_Y - 17;

      // tooltip background
      gfx_SetColor(CLR_D_GRAY);
      gfx_FillRectangle(label_x, label_y, label_w, 15);
      gfx_SetColor(CLR_P_PURPLE);
      gfx_Rectangle(label_x, label_y, label_w, 15);

      // tooltip text
      gfx::draw_text(hovered_name, label_x + 2, label_y + 3, gfx::Color::White);
    }
  }

  // --- launcher grid ---

  static void draw_launcher() {
    // dimmed background overlay
    gfx::draw_rect_filled(0, STATUS_H, GFX_LCD_WIDTH, GFX_LCD_HEIGHT - STATUS_H - DOCK_H, 0x08);

    int start = grid_page * GRID_PER_PAGE;
    int cell_w = GRID_ICON_SIZE + GRID_SPACING_X;
    int cell_h = GRID_ICON_SIZE + GRID_LABEL_H + GRID_SPACING_Y;
    int grid_w = GRID_COLS * cell_w - GRID_SPACING_X;
    int base_x = (GFX_LCD_WIDTH - grid_w) / 2;
    int base_y = GRID_TOP;

    for (int i = 0; i < GRID_PER_PAGE; i++) {
      int app_idx = start + i;
      if (app_idx >= APP_COUNT) break;

      int col = i % GRID_COLS;
      int row = i / GRID_COLS;

      int cx = base_x + col * cell_w + GRID_ICON_SIZE / 2;
      int cy = base_y + row * cell_h + GRID_ICON_SIZE / 2;

      const App& app = all_apps[app_idx];
      bool sel = (grid_sel == i);

      draw_icon(cx, cy, GRID_ICON_SIZE, app.icon_color, app.icon_glyph, sel);

      // label below icon
      int label_w = fontlib_GetStringWidth(app.name);
      int label_x = cx - label_w / 2;
      int label_y = cy + GRID_ICON_SIZE / 2 + 2;
      gfx::draw_text_small(app.name, label_x, label_y, sel ? gfx::Color::White : gfx::Color::LightGray);
    }

    // page indicator dots
    int pages = grid_page_count();
    if (pages > 1) {
      int dot_y = GFX_LCD_HEIGHT - DOCK_H - 10;
      int dots_w = pages * 8;
      int dot_x = (GFX_LCD_WIDTH - dots_w) / 2;

      for (int p = 0; p < pages; p++) {
        gfx_SetColor(p == grid_page ? gfx::Color::White : gfx::Color::DarkGray);
        gfx_FillRectangle(dot_x + p * 8, dot_y, 4, 4);
      }
    }
  }

  // --- desktop (home screen behind dock) ---

  static void draw_desktop() {
    // overlay is drawn, dont do anything
  }

  // --- update ---

  static void update_home() {
    if (hal::Input::key_left() && dock_sel > 0)
      dock_sel--;
    if (hal::Input::key_right() && dock_sel < DOCK_SLOTS - 1)
      dock_sel++;

    if (hal::Input::key_enter() || hal::Input::key_2nd()) {
      if (dock_sel == PINNED_COUNT) {
        // open launcher
        current_screen = Screen::Launcher;
        grid_sel = 0;
        grid_page = 0;
      } else {
        selected_app = pinned[dock_sel];
      }
    }

    if (hal::Input::key_clear()) {
      was_dismissed = true;
    }
  }

  static void update_launcher() {
    int col = grid_sel % GRID_COLS;
    int row = grid_sel / GRID_COLS;

    if (hal::Input::key_left()) {
      if (col > 0)
        grid_sel--;
      else if (grid_page > 0) {
        grid_page--;
        grid_sel = row * GRID_COLS + (GRID_COLS - 1);
        // clamp to last app on page
        int max_idx = grid_page * GRID_PER_PAGE + grid_sel;
        if (max_idx >= APP_COUNT)
          grid_sel = (APP_COUNT - 1) - grid_page * GRID_PER_PAGE;
      }
    }

    if (hal::Input::key_right()) {
      int next_app = grid_page * GRID_PER_PAGE + grid_sel + 1;
      if (col < GRID_COLS - 1 && next_app < APP_COUNT)
        grid_sel++;
      else if (grid_page < grid_page_count() - 1) {
        grid_page++;
        grid_sel = row * GRID_COLS;
        int max_idx = grid_page * GRID_PER_PAGE + grid_sel;
        if (max_idx >= APP_COUNT)
          grid_sel = (APP_COUNT - 1) - grid_page * GRID_PER_PAGE;
        }
    }

    if (hal::Input::key_up() && row > 0) {
      grid_sel -= GRID_COLS;
    }

    if (hal::Input::key_down()) {
      int next = grid_sel + GRID_COLS;
      int next_app = grid_page * GRID_PER_PAGE + next;
      if (next < GRID_PER_PAGE && next_app < APP_COUNT)
        grid_sel = next;
    }

    if (hal::Input::key_enter() || hal::Input::key_2nd()) {
      int app_idx = grid_page * GRID_PER_PAGE + grid_sel;
      if (app_idx < APP_COUNT) {
        selected_app = app_idx;
      }
    }

    if (hal::Input::key_clear()) {
      if (current_screen == Screen::Launcher)
        current_screen = Screen::Home;
      else
        was_dismissed = true;
    }
  }

  // --- public interface ---

  void init() {
    current_screen = Screen::Home;
    dock_sel = 0;
    grid_sel = 0;
    grid_page = 0;
    selected_app = -1;
    was_dismissed = false;
  }

  void update() {
    switch (current_screen) {
      case Screen::Home:     update_home();     break;
      case Screen::Launcher: update_launcher(); break;
    }
  }

  void draw() {
    draw_desktop();
    draw_status_bar();

    if (current_screen == Screen::Launcher)
      draw_launcher();

    draw_dock();
  }

  int get_selected_app() {
    return selected_app;
  }

  bool dismissed() {
    return was_dismissed;
  }
}