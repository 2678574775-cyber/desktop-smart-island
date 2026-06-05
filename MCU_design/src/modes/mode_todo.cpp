/**
 * TODO 模式 (per README.md)
 *   LIST:   A下一个   B进Detail   C同步
 *   DETAIL: A标记完成   B开始倒计时   C返回列表
 *   双击D: 切换 Countdown
 */

#include "mode_todo.h"
#include "../hardware/display_hal.h"
#include "../hardware/buzzer_hal.h"

void renderTodoMode() {
  display.fillRect(0, CONTENT_Y, SCREEN_W, CONTENT_H, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  struct TaskItem { const char* title, *pri, *due; int est; };
  static const TaskItem tasks[] = {
    { "1.Finish math HW",   "High", "18:00 Today",  45 },
    { "2.Review mod elec",  "Mid",  "Tomorrow",     60 },
    { "3.Prep lab report",  "Low",  "2026-06-01",   30 },
  };
  const int tc = 3;

  if (tc == 0) {
    printCentred("[ Empty ]", CONTENT_Y + 10);
    printCentred("No tasks!", CONTENT_Y + 22);
    printCentred("Add via app", CONTENT_Y + 34);
    return;
  }

  if (todoSub == TODO_DETAIL) {
    int i = todoCursor;
    display.setCursor(2, CONTENT_Y + 1); display.print(tasks[i].title);
    display.setCursor(2, CONTENT_Y + 12); display.print(F("Pri: ")); display.print(tasks[i].pri);
    display.setCursor(2, CONTENT_Y + 21); display.print(F("Due: ")); display.print(tasks[i].due);
    char b[20]; snprintf(b, sizeof(b), "Est: %d min", tasks[i].est);
    display.setCursor(2, CONTENT_Y + 30); display.print(b);
    display.drawFastHLine(2, CONTENT_Y + 38, SCREEN_W - 4, SSD1306_WHITE);
    display.setCursor(2, CONTENT_Y + 43); display.print(F("A:Mark done  B:Start CD"));
    display.setCursor(2, CONTENT_Y + 52); display.print(F("C:Back to list"));
    return;
  }

  char tt[20]; snprintf(tt, sizeof(tt), "TODO (%d)", tc);
  display.setCursor(2, CONTENT_Y); display.print(tt);
  int vs = (todoCursor >= 4) ? todoCursor - 3 : 0;
  for (int i = vs; i < tc && i < vs + 4; i++) {
    char row[22]; snprintf(row, sizeof(row), "%s", tasks[i].title);
    printLine(CONTENT_Y + 10 + (i - vs) * 9, row, i == todoCursor);
  }
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(2, CONTENT_Y + 48);
  display.print(F("A:Next  B:Detail  C:Sync"));
}

void handleTodoButtons() {
  const int tc = 3;

  if (todoSub == TODO_LIST) {
    // A: 下一个
    if (buttons[0].pressed) { todoCursor = (todoCursor + 1) % tc;
      lastUserAction = millis(); feedbackClick(); }
    // B: 进详情
    if (buttons[1].pressed) { todoSub = TODO_DETAIL;
      lastUserAction = millis(); feedbackClick(); }
    // C: 同步
    if (buttons[2].pressed) { lastUserAction = millis(); feedbackClick(); }
  }
  else if (todoSub == TODO_DETAIL) {
    // A: 标记完成
    if (buttons[0].pressed) { lastUserAction = millis(); feedbackClick(); }
    // B: 以预估时间开始倒计时
    if (buttons[1].pressed) {
      const int em[] = {45, 60, 30};
      cdMinutes = em[todoCursor]; cdState = CD_IDLE; cdIsRest = false;
      enterMode(MODE_COUNTDOWN); feedbackClick();
    }
    // C: 返回列表
    if (buttons[2].pressed) { todoSub = TODO_LIST;
      lastUserAction = millis(); feedbackClick(); }
  }
}
