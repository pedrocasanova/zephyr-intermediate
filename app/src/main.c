#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include<zephyr/task_wdt/task_wdt.h>

#define STACK_SIZE 1024
#define PRIO_HIGH  3
#define PRIO_MED   5

#define SAMPLE_COUNT 15
#define SENSOR_PERIOD 100
#define SENSOR_QUEUE_SIZE 5

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

/* MESSAGE QUEUE */
struct sensor_data {
    int voltage_mv;
    uint32_t timestamp_ms;
};

K_MSGQ_DEFINE(sensor_queue, sizeof(struct sensor_data), SENSOR_QUEUE_SIZE, 4);


/* WATCHDOG */
int chan;
void sensor_watchdog_cb(int chan_id, void *user_data)
{
    ARG_UNUSED(chan_id);
    ARG_UNUSED(user_data);
    LOG_WRN("Watchdog triggered for sensor thread");
}

/* PRODUCER */
void t_producer_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        struct sensor_data data = { 
            .voltage_mv = 18000 + (i * 100),
            .timestamp_ms = k_uptime_get_32()
        };
        int ret = k_msgq_put(&sensor_queue, &data, K_MSEC(SENSOR_PERIOD));
        if (ret != 0) {
            LOG_WRN("Producer failed with %d", ret);
            break;
        } else {
            LOG_INF("Producer queued: voltage=%d time=%d", data.voltage_mv, data.timestamp_ms);
        }
        task_wdt_feed(chan);
        k_msleep(SENSOR_PERIOD);
    }
}

/* CONSUMER */
void t_consumer_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (int i=0; i < SAMPLE_COUNT; i++) {
        struct sensor_data data;
        int ret = k_msgq_get(&sensor_queue, &data, K_FOREVER);
        if (ret != 0) {
            LOG_ERR("Processor receive failed with %d", ret);
            break;
        }
        LOG_INF("Consuming message: %d %d", data.voltage_mv, data.timestamp_ms);
        /* consumer is slower than producer to make backpressure and overflow visible */
        k_msleep(10*SENSOR_PERIOD);
    }
}

/* HEALTH MONITOR */
void t_health_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    for (int i = 0; i < 7; i++) {
        k_msleep(SENSOR_PERIOD*4);
        uint32_t queue_usage = k_msgq_num_used_get(&sensor_queue);
        LOG_INF("Health: current msg queue occupation is %d, free slots %u", queue_usage, SENSOR_QUEUE_SIZE - queue_usage);
        if ((((float) queue_usage / SENSOR_QUEUE_SIZE) * 100) > 75.0f ) {
            LOG_WRN("Health: Current message queue occupation above 75%%");
        }
    }
}

/* THREAD DECLARATION */
K_THREAD_DEFINE(t_producer, STACK_SIZE, t_producer_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(t_consumer, STACK_SIZE, t_consumer_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(t_health, STACK_SIZE, t_health_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);

int main(void)
{
    /* WATCHDOG DECLARATION */
    task_wdt_init(NULL);
    chan = task_wdt_add(2000,
                        sensor_watchdog_cb,
                        (void *) k_current_get());
    return 0;
}
