#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>

#define DAEMON_MX_CNT 64
#define DAEMON_TASK_PERIOD_MS 10U
#define DAEMON_MS_TO_TICKS(ms) ((uint16_t)(((ms) + DAEMON_TASK_PERIOD_MS - 1U) / DAEMON_TASK_PERIOD_MS))
#define DAEMON_REMIND_DISABLE 0U
#define DAEMON_REMIND_15S DAEMON_MS_TO_TICKS(15000U)

typedef enum
{
    DAEMON_EVENT_NONE = 0,
    DAEMON_EVENT_OFFLINE_ENTER,
    DAEMON_EVENT_OFFLINE_REMIND,
    DAEMON_EVENT_ONLINE_RECOVER,
} DaemonEvent_e;

typedef void (*daemon_callback)(void *, DaemonEvent_e);

struct rscf_daemon_handle;

typedef struct daemon_ins
{
    void *owner_id;
    daemon_callback callback;
    struct rscf_daemon_handle *handle;
} DaemonInstance;

typedef struct
{
    uint16_t reload_count;
    uint16_t init_count;
    uint16_t remind_count;
    daemon_callback callback;
    void *owner_id;
} Daemon_Init_Config_s;

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config);
void DaemonReload(DaemonInstance *instance);
uint8_t DaemonIsOnline(DaemonInstance *instance);
void DaemonTask(void);

#endif /* MONITOR_H */
