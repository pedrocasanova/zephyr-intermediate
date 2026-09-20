#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/tracing/tracing.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_INF);

#define STACK_SIZE            2048
#define CONTROL_PRIORITY         4   /* CORRECTED: now higher priority than maintenance */
#define MAINTENANCE_PRIORITY     7   /* CORRECTED: now lower priority than control */
#define EVENT_PERIOD_MS        250
#define MAINTENANCE_LOAD_US  45000
#define DEADLINE_MS              10  /* response-time guarantee from main()'s log message */

struct control_event {
    uint32_t seq;
    uint32_t ready_ms;
};

K_MSGQ_DEFINE(control_queue, sizeof(struct control_event), 4, 4);
K_SEM_DEFINE(maintenance_start, 0, 1);

static uint32_t deadline_misses;

static void event_timer_expiry(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    static uint32_t seq;
    struct control_event event = {
        .seq = seq++,
        .ready_ms = k_uptime_get_32(),
    };

    int ret = k_msgq_put(&control_queue, &event, K_NO_WAIT);

    if (ret != 0) {
        return;
    }

    k_sem_give(&maintenance_start);

    sys_trace_named_event("event_ready", event.seq, 0);
}

K_TIMER_DEFINE(event_timer, event_timer_expiry, NULL);

static void control_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (true) {
        struct control_event event;
        int ret = k_msgq_get(&control_queue, &event, K_FOREVER);

        if (ret != 0) {
            LOG_ERR("[CONTROL] receive failed: %d", ret);
            continue;
        }

        uint32_t latency_ms = k_uptime_get_32() - event.ready_ms;

        LOG_INF("[CONTROL] processed seq=%u latency_ms=%u", event.seq, latency_ms);

        if (latency_ms > DEADLINE_MS) {
            deadline_misses++;
            LOG_WRN_RATELIMIT(
                "deadline_miss count=%u seq=%u latency_ms=%u",
                deadline_misses, event.seq, latency_ms);
        }

        sys_trace_named_event("event_done", event.seq, latency_ms);
    }
}

static void maintenance_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (true) {
        k_sem_take(&maintenance_start, K_FOREVER);
        k_busy_wait(MAINTENANCE_LOAD_US);
    }
}

K_THREAD_DEFINE(control, STACK_SIZE, control_fn,
                NULL, NULL, NULL, CONTROL_PRIORITY, 0, 0);

K_THREAD_DEFINE(maintenance, STACK_SIZE, maintenance_fn,
                NULL, NULL, NULL, MAINTENANCE_PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("=== L6 Homework: Runtime Investigation ===");
    LOG_INF("Control work must start within 10 ms");
    LOG_INF("Inspect, measure, trace, explain, and correct the delay");

    k_timer_start(&event_timer, K_MSEC(500), K_MSEC(EVENT_PERIOD_MS));

    return 0;
}
