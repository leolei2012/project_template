#include "alarm.h"
#include <string.h>

static uint8_t alarm_check_condition(const struct alarm *cb, uint16_t current_value)
{
    if (cb == NULL)
    {
        return 0;
    }

    switch (cb->threshold_type)
    {
    case GREATER_OR_EQUAL_THAN_THRESHOLD:
        return (current_value >= cb->threshold_value) ? 1 : 0;

    case GREATER_THAN_THRESHOLD:
        return (current_value > cb->threshold_value) ? 1 : 0;

    case LESS_OR_EQUAL_THAN_THRESHOLD:
        return (current_value <= cb->threshold_value) ? 1 : 0;

    case LESS_THAN_THRESHOLD:
        return (current_value < cb->threshold_value) ? 1 : 0;

    case EQUAL_TO_THRESHOLD:
        return (current_value == cb->threshold_value) ? 1 : 0;

    case NOT_EQUAL_TO_THRESHOLD:
        return (current_value != cb->threshold_value) ? 1 : 0;

    default:
        return 0;
    }
}

static uint8_t alarm_check_recovery(const struct alarm *cb, uint16_t current_value)
{
    uint16_t recovery_threshold;

    if (cb == NULL)
    {
        return 0;
    }

    recovery_threshold = 0;

    /// 根据告警类型计算恢复阈值（带回差）
    switch (cb->threshold_type)
    {
    case GREATER_OR_EQUAL_THAN_THRESHOLD:
    case GREATER_THAN_THRESHOLD:
        /// 对于大于类型的告警，恢复需要小于(阈值-回差值)
        recovery_threshold = cb->threshold_value - cb->recovery_diff_value;
        return (current_value < recovery_threshold) ? 1 : 0;

    case LESS_OR_EQUAL_THAN_THRESHOLD:
    case LESS_THAN_THRESHOLD:
        /// 对于小于类型的告警，恢复需要大于(阈值+回差值)
        recovery_threshold = cb->threshold_value + cb->recovery_diff_value;
        return (current_value > recovery_threshold) ? 1 : 0;

    case EQUAL_TO_THRESHOLD:
        /// 对于等于类型的告警，恢复需要不等于阈值
        return (current_value != cb->threshold_value) ? 1 : 0;

    case NOT_EQUAL_TO_THRESHOLD:
        /// 对于不等于类型的告警，恢复需要等于阈值
        return (current_value == cb->threshold_value) ? 1 : 0;

    default:
        return 0;
    }
}

/**
 * @brief 初始化告警对象
 */
void alarm_init(struct alarm *cb)
{
    if (cb == NULL)
    {
        return;
    }

    memset(cb, 0, sizeof(*cb));
    cb->status = ALARM_STATUS_NORMAL;
    cb->enabled = 0;
}

int8_t alarm_config(struct alarm *cb, const struct alarm_config_t *config)
{
    if (cb == NULL || config == NULL)
    {
        return -1;
    }

    if (config->data_source == NULL)
    {
        return -1;
    }

    cb->alarm_id = config->alarm_id;
    cb->threshold_type = config->threshold_type;
    cb->threshold_value = config->threshold_value;
    cb->recovery_diff_value = config->recovery_diff_value;
    cb->data_source = config->data_source;
    cb->duration = config->duration;
    cb->recovery_time = config->recovery_time;
    cb->callback = config->callback; /// 保存回调函数

    /// 清除运行状态
    cb->elapsed_time = 0;
    cb->recovery_elapsed_time = 0;
    cb->status = ALARM_STATUS_NORMAL;
    cb->enabled = 1; /// 配置后自动使能

    return 0;
}

void alarm_run(struct alarm *cb)
{
    uint16_t current_value;

    if (cb == NULL || !cb->enabled || cb->data_source == NULL)
    {
        return;
    }

    current_value = *(cb->data_source);

    /// 根据当前状态执行不同的逻辑
    switch (cb->status)
    {
    case ALARM_STATUS_NORMAL:
    {
        /// 检查是否满足告警条件
        if (alarm_check_condition(cb, current_value))
        {
            cb->elapsed_time++;

            /// 持续满足条件达到设定时长，触发告警
            if (cb->elapsed_time >= cb->duration)
            {
                cb->status = ALARM_STATUS_ACTIVE;
                cb->elapsed_time = 0;
                cb->recovery_elapsed_time = 0;


                cb->trigger_count++;


                if (cb->callback != NULL)
                {
                    cb->callback(cb->alarm_id, ALARM_EVENT_TRIGGERED);
                }


            }
        }
        else
        {
            /// 条件不满足，重置计时
            cb->elapsed_time = 0;
        }
        break;
    }

    case ALARM_STATUS_ACTIVE:
    {
        /// 检查是否满足恢复条件
        if (alarm_check_recovery(cb, current_value))
        {
            cb->recovery_elapsed_time++;

            /// 持续满足恢复条件达到设定时长，告警恢复
            if (cb->recovery_elapsed_time >= cb->recovery_time)
            {
                cb->status = ALARM_STATUS_NORMAL;
                cb->elapsed_time = 0;
                cb->recovery_elapsed_time = 0;

                if (cb->callback != NULL)
                {
                    cb->callback(cb->alarm_id, ALARM_EVENT_RECOVERED);
                }


            }
        }
        else
        {
            /// 恢复条件不满足，重置恢复计时
            cb->recovery_elapsed_time = 0;
        }
        break;
    }

    case ALARM_STATUS_DISABLED:
    default:
        /// 禁用状态，不做处理
        break;
    }
}

void alarm_enable(struct alarm *cb)
{
    if (cb == NULL)
    {
        return;
    }

    cb->enabled = 1;
    if (cb->status == ALARM_STATUS_DISABLED)
    {
        cb->status = ALARM_STATUS_NORMAL;
    }
}

void alarm_disable(struct alarm *cb)
{
    if (cb == NULL)
    {
        return;
    }

    cb->enabled = 0;
    cb->status = ALARM_STATUS_DISABLED;
    cb->elapsed_time = 0;
    cb->recovery_elapsed_time = 0;
}

void alarm_clear(struct alarm *cb)
{
    if (cb == NULL)
    {
        return;
    }

    cb->status = ALARM_STATUS_NORMAL;
    cb->elapsed_time = 0;
    cb->recovery_elapsed_time = 0;
}

enum alarm_status_t alarm_get_status(const struct alarm *cb)
{
    if (cb == NULL)
    {
        return ALARM_STATUS_DISABLED;
    }

    return cb->status;
}