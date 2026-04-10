#include <stdlib.h>
#include <string.h>

#include <rscf/rscf_daemon_service.h>

#include "daemon.h"

static DaemonEvent_e DaemonMapEvent(enum rscf_daemon_event event)
{
    switch (event)
    {
    case RSCF_DAEMON_EVENT_OFFLINE_ENTER:
        return DAEMON_EVENT_OFFLINE_ENTER;
    case RSCF_DAEMON_EVENT_OFFLINE_REMIND:
        return DAEMON_EVENT_OFFLINE_REMIND;
    case RSCF_DAEMON_EVENT_ONLINE_RECOVER:
        return DAEMON_EVENT_ONLINE_RECOVER;
    case RSCF_DAEMON_EVENT_NONE:
    default:
        return DAEMON_EVENT_NONE;
    }
}

static void DaemonCompatCallback(void *owner, enum rscf_daemon_event event)
{
    DaemonInstance *instance = (DaemonInstance *)owner;

    if ((instance == NULL) || (instance->callback == NULL))
    {
        return;
    }

    instance->callback(instance->owner_id, DaemonMapEvent(event));
}

DaemonInstance *DaemonRegister(Daemon_Init_Config_s *config)
{
    struct rscf_daemon_config daemon_config;
    DaemonInstance *instance;

    if (config == NULL)
    {
        return NULL;
    }

    instance = (DaemonInstance *)malloc(sizeof(DaemonInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    memset(instance, 0, sizeof(DaemonInstance));
    instance->owner_id = config->owner_id;
    instance->callback = config->callback;

    daemon_config.reload_ticks = config->reload_count;
    daemon_config.init_ticks = config->init_count;
    daemon_config.remind_ticks = config->remind_count;
    daemon_config.callback = DaemonCompatCallback;
    daemon_config.owner = instance;
    daemon_config.name = NULL;
    if (RSCFDaemonRegister(&daemon_config, &instance->handle) != 0)
    {
        free(instance);
        return NULL;
    }

    return instance;
}

void DaemonReload(DaemonInstance *instance)
{
    if (instance == NULL)
    {
        return;
    }

    RSCFDaemonReload(instance->handle);
}

uint8_t DaemonIsOnline(DaemonInstance *instance)
{
    if (instance == NULL)
    {
        return 0U;
    }

    return RSCFDaemonIsOnline(instance->handle) ? 1U : 0U;
}

void DaemonTask(void)
{
    /* Zephyr 版本由守护服务内部周期调度，这里保留兼容空实现。 */
}
