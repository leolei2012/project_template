#ifndef APP_KEY_H
#define APP_KEY_H

#include "platform.h"

enum app_key_event_t
{
    APP_KEY_EVENT_NONE = 0,
    APP_KEY_EVENT_BEEP_SHORT,
    APP_KEY_EVENT_BEEP_FOR_CHILD_LOCK,
    APP_KEY_EVENT_BEEP_2_TIMES,
};

enum app_key_state_t
{
    APP_KEY_STATE_INIT = 0,
    APP_KEY_STATE_INIT_OK,
    APP_KEY_STATE_RUNNING
};

struct app_key
{
    uint8_t gesture_owner;         /// 当前占用按键序列的手势
    uint8_t gesture_hold_mask;     /// 正在被高优先级手势占用的按键
    uint8_t gesture_suppress_mask; /// 松手后需要转入 click 抑制的按键
    uint8_t suppress_click_mask;   /// 需要吞掉一次 CLICKED 的按键

    uint16_t humidity_long_press_tick;    /// 湿度长按连续触发计数器
    uint16_t humidity_long_press_started; /// 湿度长按是否已开始标志
    uint16_t combo_key1_tick;             /// 组合键1计时器（风速+湿度）
    uint16_t combo_key1_triggered;        /// 组合键1是否已触发标志
    uint16_t combo_key2_tick;             /// 组合键2计时器（定时+湿度）
    uint16_t combo_key2_triggered;        /// 组合键2是否已触发标志
    uint16_t combo_key3_tick;             /// 组合键3计时器（调速+电源）
    uint16_t combo_key3_triggered;        /// 组合键3是否已触发标志
    uint16_t combo_key4_tick;             /// 组合键4计时器（调速+定时）
    uint16_t combo_key4_triggered;        /// 组合键4是否已触发标志
    uint16_t speed_long_press_tick;       /// 调速键单独长按计时器
    uint16_t speed_long_press_triggered;  /// 调速键长按是否已触发标志
    uint16_t timer_long_press_tick;       /// 定时键单独长按计时器
    uint16_t timer_long_press_triggered;  /// 定时键长按是否已触发标志
    uint16_t power_long_press_tick;       /// 电源键单独长按计时器
    uint16_t power_long_press_triggered;  /// 电源键长按是否已触发标志

    uint8_t key_event; /// 0： 1： 2：
};

void app_key_init(struct app_key *cb);
void app_key_poll(struct app_key *cb);

#endif // APP_KEY_H