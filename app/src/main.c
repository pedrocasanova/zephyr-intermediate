#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "sensor_iface.h"

#define STACK_SIZE 1024
#define PRIO_HIGH  3

#define SENSOR_COUNT 15
#define SENSOR_PERIOD 100

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

/* PUBLISHER */
void t_sensor_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        struct sensor_data data = { 
            .voltage_mv = 18000 + (i * 100),
            .timestamp_ms = k_uptime_get_32()
        };
        int ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(SENSOR_PERIOD));
        if (ret != 0) {
            LOG_WRN("Sensor publish failed with %d", ret);
        } else {
            LOG_INF("Sensor publish: voltage=%d time=%d", data.voltage_mv, data.timestamp_ms);
        }
        k_msleep(SENSOR_PERIOD);
    }


}

/* MOCKUP FUNCTION TO UPDATE A DISPLAY */
void display_update(int32_t value) {return;}

/* LISTENER */
void display_cb(const struct zbus_channel *chan)
{
    const struct sensor_data *msg = zbus_chan_const_msg(chan);
    display_update(msg->voltage_mv);
}

ZBUS_LISTENER_DEFINE(display_lis, display_cb);

ZBUS_SUBSCRIBER_DEFINE(logger_sub, 4);

/* SUBSCRIBER */
void t_logger_fn(void)
{
    const struct zbus_channel *chan;
    while (!zbus_sub_wait(&logger_sub, &chan, K_FOREVER)) {
        struct sensor_data msg;
        zbus_chan_read(chan, &msg, K_MSEC(100));
        LOG_INF("Subscriber log: Voltage: %d mV, Timestamp: %d", msg.voltage_mv, msg.timestamp_ms);
    }
}

K_THREAD_DEFINE(t_sensor, STACK_SIZE, t_sensor_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(t_logger, STACK_SIZE, t_logger_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);

int main(void)
{
    return 0;
}
