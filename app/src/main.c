#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_LOW   7
#define PRIO_MED   5
#define PRIO_HIGH  3
#define PRIO_COOP  -1

static volatile uint16_t counter = 0u;

static K_MUTEX_DEFINE(counter_mutex);

void ctr_fn(void *p1, void *p2, void *p3)
{
    k_mutex_lock(&counter_mutex, K_FOREVER);
    counter++;
    k_mutex_unlock(&counter_mutex);
    LOG_INF("Counter is %d", counter);
}

K_THREAD_DEFINE(t1,  STACK_SIZE, ctr_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);
K_THREAD_DEFINE(t2,  STACK_SIZE, ctr_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);

int main(void)
{
    while (1) {

    };

    return 0;
}
