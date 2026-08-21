#ifndef APP_ALARM_SYSTEM_H
#define APP_ALARM_SYSTEM_H

#include "platform.h"
#include "alarm.h"

enum app_alarm_system_id_t
{
    /// NOMAL ALARMS
    APP_ALARM_SYSTEM_ID_EVA_TEMP_SENSOR_FAILURE,     /// EVA温度传感器故障/盘管温度传感器故障/蒸发器温度传感器故障 -> E1
    
    APP_ALARM_SYSTEM_ID_HUMIDITY_SENSOR_FAILURE,     /// 环境湿度传感器故障 -> E2
    APP_ALARM_SYSTEM_ID_AMBIENT_TEMP_SENSOR_FAILURE, /// 环境温度传感器故障 -> E2

    APP_ALARM_SYSTEM_ID_WIFI_DISCONNECTED,           /// WIFI通信故障 -> E3

    APP_ALARM_SYSTEM_ID_INVERTER_DISCONNECTED,       /// 变频器通信故障 -> E4
    
    APP_ALARM_SYSTEM_ID_MEMORY_FAILURE,              /// 存储器故障 -> E6

    /// SPECIAL ALARMS
    APP_ALARM_SYSTEM_ID_INLET_TEMP_OVERHEAT,          /// 进风口温度过高 -> L3
    APP_ALARM_SYSTEM_ID_INLET_TEMP_SUPERCOOL,         /// 进风口温度过低 -> L4
    APP_ALARM_SYSTEM_ID_AMBIENT_HUM_UNDER_30_PERCENT, /// 环境湿度≤30% -> LO
    APP_ALARM_SYSTEM_ID_AMBIENT_HUM_OVER_90_PERCENT,  /// 环境湿度≥90% -> HI

    APP_ALARM_SYSTEM_ID_NUM,
};


struct app_alarm_system
{
    struct alarm alarms[APP_ALARM_SYSTEM_ID_NUM]; /// 告警对象数组
    uint8_t active_alarm_count;                   /// 当前活动的告警数量

#if 0
    uint32_t total_alarm_count; /// 总告警触发次数
    uint32_t system_run_count;  /// 系统运行次数统计
#endif
};

/**
 * @brief 初始化告警系统
 * @param cb 告警系统对象指针
 */
void app_alarm_system_init(struct app_alarm_system *cb);

/**
 * @brief 注册告警
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @param config 告警配置参数
 * @return 0=成功, -1=失败
 */
int8_t app_alarm_system_register(struct app_alarm_system *cb, const struct alarm_config_t *config);

/**
 * @brief 运行告警系统（需要周期性调用）
 * @param cb 告警系统对象指针
 * @note 建议在定时器中断或主循环中周期调用
 */
void app_alarm_system_run(struct app_alarm_system *cb);

/**
 * @brief 使能指定告警
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @return 0=成功, -1=失败
 */
int8_t app_alarm_system_enable_alarm(struct app_alarm_system *cb, uint8_t alarm_id);

/**
 * @brief 禁用指定告警
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @return 0=成功, -1=失败
 */
int8_t app_alarm_system_disable_alarm(struct app_alarm_system *cb, uint8_t alarm_id);

/**
 * @brief 清除指定告警状态
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @return 0=成功, -1=失败
 */
int8_t app_alarm_system_clear_alarm(struct app_alarm_system *cb, uint8_t alarm_id);

/**
 * @brief 清除所有告警状态
 * @param cb 告警系统对象指针
 */
void app_alarm_system_clear_all(struct app_alarm_system *cb);

/**
 * @brief 获取指定告警的状态
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @return 告警状态
 */
enum alarm_status_t app_alarm_system_get_status(const struct app_alarm_system *cb, uint8_t alarm_id);

/**
 * @brief 检查是否有活动告警
 * @param cb 告警系统对象指针
 * @return 1=有活动告警, 0=无活动告警
 */
uint8_t app_alarm_system_has_active_alarms(const struct app_alarm_system *cb);

/**
 * @brief 获取活动告警数量
 * @param cb 告警系统对象指针
 * @return 活动告警数量
 */
uint8_t app_alarm_system_get_active_count(const struct app_alarm_system *cb);

/**
 * @brief 获取告警对象指针
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @return 告警对象指针，失败返回NULL
 */
struct alarm *app_alarm_system_get_alarm(struct app_alarm_system *cb, uint8_t alarm_id);

/**
 * @brief 获取告警触发次数
 * @param cb 告警系统对象指针
 * @param alarm_id 告警ID
 * @return 告警触发次数
 */
uint32_t app_alarm_system_get_trigger_count(const struct app_alarm_system *cb, uint8_t alarm_id);

#define APP_ALARM_SYSTEM_TASK_PERIOD 1000
void app_alarm_system_poll(struct app_alarm_system *cb);

#endif // APP_ALARM_SYSTEM_H