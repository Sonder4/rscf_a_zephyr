#ifndef IMU_H_
#define IMU_H_

#include <stdint.h>

#include <zephyr/kernel.h>

#include "bsp_usart.h"

#define SABER_IMU_TOPIC_NAME "saber_imu"

typedef struct
{
    float CalXaxis_Data;
    float CalYaxis_Data;
    float CalZaxis_Data;
} Cal_Data;

typedef struct
{
    float Xaxis_Data;
    float Yaxis_Data;
    float Zaxis_Data;
} Kal_Data;

typedef struct
{
    float roll;
    float pitch;
    float yaw;
} CapeEuler_Data_t;

typedef struct
{
    Kal_Data kalman_acc_data;
    Kal_Data kalman_gyro_data;
    CapeEuler_Data_t euler_data;
    uint32_t update_tick;
    uint32_t sample_seq;
} SaberImuTopicData_t;

typedef struct
{
    SaberImuTopicData_t frame;
    volatile uint32_t latest_seq;
    volatile uint8_t valid;
} SaberLatestMailbox_t;

typedef struct
{
    USARTInstance *usart_instance;
    Kal_Data kalman_acc_data;
    Kal_Data kalman_gyro_data;
    CapeEuler_Data_t euler_data;
    SaberLatestMailbox_t latest_mailbox;
    uint32_t last_acquired_seq;
    uint8_t data_updated_flag;
    uint8_t rx_buff[60];
} SaberInstance;

typedef struct
{
    UART_HandleTypeDef *usart_handle;
} Saber_Init_Config_s;

typedef k_tid_t TaskHandle_t;

extern SaberInstance *g_saber_instance;
extern TaskHandle_t g_saber_imu_topic_task_handle;
extern CapeEuler_Data_t g_euler_data;

CapeEuler_Data_t SaberGetEuler(SaberInstance *instance);
uint8_t SaberAcquire(SaberInstance *instance, CapeEuler_Data_t *data_store);
uint8_t SaberTryGetLatestTopicData(SaberImuTopicData_t *out);
void SaberTopicInit(void);
void SaberSetTopicTaskHandle(TaskHandle_t task_handle);
void SaberPublishTask(void *argument);
void SaberInit(void);

#endif /* IMU_H_ */
