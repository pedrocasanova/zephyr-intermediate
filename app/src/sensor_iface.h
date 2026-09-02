#include <zephyr/zbus/zbus.h>

/* SENSOR STRUCT */
struct sensor_data {
	int32_t voltage_mv;
	uint32_t timestamp_ms;
};

/* CHANNEL */
ZBUS_CHAN_DEFINE(
    sensor_chan,
    struct sensor_data,
    NULL, NULL,
    ZBUS_OBSERVERS(display_lis, logger_sub),
    ZBUS_MSG_INIT(.voltage_mv = 0)
);
