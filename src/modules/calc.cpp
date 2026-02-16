#include "modules/calc.h"
#include "gfx/gfx.h"
#include "hal/input.h"
#include "config.h"

#include <graphx.h>
#include <keypadc.h>
#include <fontlibc.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

// --- layout ---

static constexpr int TOP_BAR_H   = 14;
static constexpr int LINE_H      = 22;
static constexpr int MARGIN_X    = 6;
static constexpr int MAX_EXPR    = 64;
static constexpr int MAX_HISTORY = 8;
static constexpr int PREVIEW_Y   = SCREEN_H - 16;
static constexpr int INPUT_Y     = SCREEN_H - 34;

// --- history entry ---

struct HistoryEntry {
  char expr[MAX_EXPR];
  char result[MAX_EXPR];
};

static HistoryEntry history[MAX_HISTORY];
static int history_count = 0;

// --- current input buffer ---

static char input_buf[MAX_EXPR];
static int  input_len = 0;
static int  cursor = 0;

// --- expression parser ---

static const char* parse_ptr;
static bool parse_error;

static double parse_expr();
static double parse_term();
static double parse_factor();
static double parse_unary();
static double parse_primary();

static void skip_spaces() {
  while (*parse_ptr == ' ') parse_ptr++;
}

static double parse_expr() {
  double val = parse_term();
  while (!parse_error) {
    skip_spaces();
    if (*parse_ptr == '+') { parse_ptr++; val += parse_term(); }
    else if (*parse_ptr == '-') { parse_ptr++; val -= parse_term(); }
    else break;
  }
  return val;
}

static double parse_term() {
  double val = parse_unary();
  while (!parse_error) {
    skip_spaces();
    if (*parse_ptr == '*') { parse_ptr++; val *= parse_unary(); }
    else if (*parse_ptr == '/') {
      parse_ptr++;
      double d = parse_unary();
      if (d == 0.0) { parse_error = true; return 0; }
      val /= d;
    }
    else break;
  }
  return val;
}

static double parse_unary() {
  skip_spaces();
  if (*parse_ptr == '-') {
    parse_ptr++;
    return -parse_unary();
  }
  return parse_primary();
}

static double parse_primary() {
  skip_spaces();

  if (*parse_ptr == '(') {
    parse_ptr++;
    double val = parse_expr();
    skip_spaces();
    if (*parse_ptr == ')') parse_ptr++;
    else parse_error = true;
    return val;
  }

  if ((*parse_ptr >= '0' && *parse_ptr <= '9') || *parse_ptr == '.') {
    char* end;
    double val = strtod(parse_ptr, &end);
    if (end == parse_ptr) { parse_error = true; return 0; }
    parse_ptr = end;
    return val;
  }

  parse_error = true;
  return 0;
}

static bool evaluate(const char* expr, double* out) {
  if (expr[0] == '\0') return false;

  parse_ptr = expr;
  parse_error = false;

  double result = parse_expr();

  skip_spaces();
  if (*parse_ptr != '\0') parse_error = true;

  if (parse_error) return false;

  *out = result;
  return true;
}

// --- format result ---

static void format_result(double val, char* buf, int buf_size) {
  // if int, show w/o decimals
  if (val == (double)(long long)val && fabs(val) < 1e15) {
    snprintf(buf, buf_size, "%lld", (long long)val);
  } else {
    snprintf(buf, buf_size, "%.10g", val);
  }
}

// --- input handling ---

static void input_clear() {
  input_len = 0;
  cursor = 0;
  input_buf[0] = '\0';
}

static void input_insert(char c) {
  if (input_len >= MAX_EXPR - 1) return;

  // shift chars right to make room at cursor
  for (int i = input_len; i > cursor; i--)
    input_buf[i] = input_buf[i - 1];
  
  input_buf[cursor] = c;
  input_len++;
  cursor++;
  input_buf[input_len] = '\0';
}

static void input_backspace() {
  if (cursor <= 0) return;

  for (int i = cursor - 1; i < input_len - 1; i++)
    input_buf[i] = input_buf[i + 1];

  input_len--;
  cursor--;
  input_buf[input_len] = '\0';
}

static void commit_input() {
  if (input_len == 0) return;

  double result;
  bool ok = evaluate(input_buf, &result);

  // shift history up if full
  if (history_count >= MAX_HISTORY) {
    for (int i = 0; i < MAX_HISTORY - 1; i++)
      history[i] = history[i + 1];
    history_count = MAX_HISTORY - 1;
  }

  HistoryEntry& entry = history[history_count];
  strncpy(entry.expr, input_buf, MAX_EXPR - 1);
  entry.expr[MAX_EXPR - 1] = '\0';

  if (ok)
    format_result(result, entry.result, MAX_EXPR);
  else
    strncpy(entry.result, "Error", MAX_EXPR);
  
  history_count++;
  input_clear();
}

// --- key polling ---

static void poll_keys() {
  // digits
  if (hal::Input::key_pressed(3, kb_0))      input_insert('0');
  if (hal::Input::key_pressed(3, kb_1))      input_insert('1');
  if (hal::Input::key_pressed(4, kb_2))      input_insert('2');
  if (hal::Input::key_pressed(5, kb_3))      input_insert('3');
  if (hal::Input::key_pressed(3, kb_4))      input_insert('4');
  if (hal::Input::key_pressed(4, kb_5))      input_insert('5');
  if (hal::Input::key_pressed(5, kb_6))      input_insert('6');
  if (hal::Input::key_pressed(3, kb_7))      input_insert('7');
  if (hal::Input::key_pressed(4, kb_8))      input_insert('8');
  if (hal::Input::key_pressed(5, kb_9))      input_insert('9');
  if (hal::Input::key_pressed(3, kb_DecPnt)) input_insert('.');

  // operators
  if (hal::Input::key_pressed(6, kb_Add))    input_insert('+');
  if (hal::Input::key_pressed(6, kb_Sub))    input_insert('-');
  if (hal::Input::key_pressed(6, kb_Mul))    input_insert('*');
  if (hal::Input::key_pressed(6, kb_Div))    input_insert('/');

  // parenthesis
  if (hal::Input::key_pressed(4, kb_LParen)) input_insert('(');
  if (hal::Input::key_pressed(5, kb_RParen)) input_insert(')');

  // negation ([-] key)
  if (hal::Input::key_pressed(5, kb_Chs))    input_insert('-');

  // cursor movement
  if (hal::Input::key_left()  && cursor > 0)          cursor--;
  if (hal::Input::key_right() && cursor < input_len)  cursor++;

  // delete
  if (hal::Input::key_pressed(1, kb_Del))    input_backspace();
  
  // enter
  if (hal::Input::key_pressed(6, kb_Enter))     commit_input();

  if (hal::Input::key_clear()) {
    if (input_len > 0)        input_clear();
    else { history_count = 0; input_clear(); }
  }
}

// --- drawing ---

static void draw_top_bar() {
  gfx::draw_rect_filled(0, 0, SCREEN_W, TOP_BAR_H, 0x4A);
  gfx::draw_text("Calculator", 4, 2, gfx::Color::White);
  gfx::draw_text("[clear] exit", SCREEN_W - 100, 2, gfx::Color::DarkGray);
}

static void draw_history() {
  // draw history from bottom up
  int y_base = INPUT_Y - 8;

  int start = history_count > MAX_HISTORY ? history_count - MAX_HISTORY : 0;
  int visible = history_count - start;

  for (int i = visible - 1; i >= 0; i--) {
    int idx = start + i;
    int y = y_base - (visible - i) * (LINE_H * 2);

    if (y < TOP_BAR_H) break;

    // expression (l-align)
    gfx::draw_text(history[idx].expr, MARGIN_X, y, gfx::Color::Black);

    // result (r-align, next line)
    int result_w = (int)strlen(history[idx].result) * 8;
    int result_x = SCREEN_W - MARGIN_X - result_w;
    gfx::draw_text(history[idx].result, result_x, y + LINE_H, gfx::Color::Black);
  }
}

static void draw_input_line() {
  gfx::draw_rect_filled(0, INPUT_Y - 2, SCREEN_W, 18, gfx::Color::White);

  gfx::draw_text(input_buf, MARGIN_X, INPUT_Y, gfx::Color::Black);

  char tmp[MAX_EXPR];
  strncpy(tmp, input_buf, cursor);
  tmp[cursor] = '\0';
  int cursor_x = MARGIN_X + fontlib_GetStringWidth(tmp);
  gfx_SetColor(gfx::Color::Black);
  gfx_FillRectangle(cursor_x, INPUT_Y + 9, 7, 2);
}

static void draw_preview() {
  if (input_len == 0) return;

  double result;
  bool ok = evaluate(input_buf, &result);

  if (ok) {
    char buf[MAX_EXPR];
    format_result(result, buf, MAX_EXPR);

    int result_w = (int)strlen(buf) * 8;
    int result_x = SCREEN_W - MARGIN_X - result_w;

    gfx::draw_text(buf, result_x, PREVIEW_Y, gfx::Color::LightGray);
  }
}

// --- separator line ---

static void draw_separator() {
  gfx_SetColor(gfx::Color::DarkGray);
  gfx_HorizLine(MARGIN_X, INPUT_Y - 4, SCREEN_W - MARGIN_X * 2);
}

// --- public interface ---

static void calc_init() {
  input_clear();
  history_count = 0;
}

static AppAction calc_tick() {
  poll_keys();

  gfx_FillScreen(gfx::Color::White);
  draw_top_bar();
  draw_history();
  draw_separator();
  draw_input_line();
  draw_preview();

  return AppAction::Continue;
}

static void calc_shutdown() {
  // nothing to clean up
}

const AppInterface calc_app = { calc_init, calc_tick, calc_shutdown };