#include <stdlib.h>
#include <string.h>

#include "IMU.h"
#include "rscf_imu_uart.h"

SaberInstance *g_saber_instance;
TaskHandle_t g_saber_imu_topic_task_handle;
CapeEuler_Data_t g_euler_data;

static void RSCFSaberSyncFromLatest(SaberInstance *instance)
{
    struct rscf_saber_topic_data latest = {0};

    if ((instance == NULL) || (!RSCFImuUartTryGetLatest(&latest)))
    {
        return;
    }

    instance->kalman_acc_data.Xaxis_Data = latest.kalman_acc.x;
    instance->kalman_acc_data.Yaxis_Data = latest.kalman_acc.y;
    instance->kalman_acc_data.Zaxis_Data = latest.kalman_acc.z;
    instance->kalman_gyro_data.Xaxis_Data = latest.kalman_gyro.x;
    instance->kalman_gyro_data.Yaxis_Data = latest.kalman_gyro.y;
    instance->kalman_gyro_data.Zaxis_Data = latest.kalman_gyro.z;
    instance->euler_data.roll = latest.euler.roll;
    instance->euler_data.pitch = latest.euler.pitch;
    instance->euler_data.yaw = latest.euler.yaw;
    instance->latest_mailbox.frame.kalman_acc_data = instance->kalman_acc_data;
    instance->latest_mailbox.frame.kalman_gyro_data = instance->kalman_gyro_data;
    instance->latest_mailbox.frame.euler_data = instance->euler_data;
    instance->latest_mailbox.frame.update_tick = latest.update_tick;
    instance->latest_mailbox.frame.sample_seq = latest.sample_seq;
    instance->latest_mailbox.latest_seq = latest.sample_seq;
    instance->latest_mailbox.valid = 1U;
    instance->data_updated_flag = 1U;
    g_euler_data = instance->euler_data;
}

CapeEuler_Data_t SaberGetEuler(SaberInstance *instance)
{
    CapeEuler_Data_t empty = {0};

    if (instance == NULL)
    {
        return empty;
    }

    RSCFSaberSyncFromLatest(instance);
    return instance->euler_data;
}

uint8_t SaberAcquire(SaberInstance *instance, CapeEuler_Data_t *data_store)
{
    if ((instance == NULL) || (data_store == NULL))
    {
        return 0U;
    }

    RSCFSaberSyncFromLatest(instance);
    if ((!instance->latest_mailbox.valid) || (instance->last_acquired_seq == instance->latest_mailbox.latest_seq))
    {
        return 0U;
    }

    *data_store = instance->euler_data;
    instance->last_acquired_seq = instance->latest_mailbox.latest_seq;
    instance->data_updated_flag = 0U;
    return 1U;
}

uint8_t SaberTryGetLatestTopicData(SaberImuTopicData_t *out)
{
    if ((g_saber_instance == NULL) || (out == NULL))
    {
        return 0U;
    }

    RSCFSaberSyncFromLatest(g_saber_instance);
    if (!g_saber_instance->latest_mailbox.valid)
    {
        return 0U;
    }

    *out = g_saber_instance->latest_mailbox.frame;
    return 1U;
}

void SaberTopicInit(void)
{
}

void SaberSetTopicTaskHandle(TaskHandle_t task_handle)
{
    g_saber_imu_topic_task_handle = task_handle;
}

void SaberPublishTask(void *argument)
{
    ARG_UNUSED(argument);
}

void SaberInit(void)
{
    if (g_saber_instance != NULL)
    {
        return;
    }

    g_saber_instance = (SaberInstance *)malloc(sizeof(SaberInstance));
    if (g_saber_instance == NULL)
    {
        return;
    }

    memset(g_saber_instance, 0, sizeof(SaberInstance));
    RSCFSaberSyncFromLatest(g_saber_instance);
}
