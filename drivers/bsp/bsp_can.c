#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bsp_can.h"

LOG_MODULE_REGISTER(bsp_can, LOG_LEVEL_INF);

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)

static CANInstance *s_can_instances[CAN_MX_REGISTER_CNT];
static uint8_t s_can_instance_count;
static bool s_can_service_inited;
static const struct device *s_can1_dev;
static const struct device *s_can2_dev;

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

static void BSPCANRxCallback(const struct device *dev, struct can_frame *frame, void *user_data)
{
    CANInstance *instance = (CANInstance *)user_data;

    ARG_UNUSED(dev);
    if ((instance == NULL) || (frame == NULL))
    {
        return;
    }

    instance->rx_len = frame->dlc;
    memcpy(instance->rx_buff, frame->data, frame->dlc);

    if (instance->can_module_callback != NULL)
    {
        instance->can_module_callback(instance);
    }
}

static const struct device *BSPCANResolveDevice(CAN_HandleTypeDef *handle)
{
    if (handle == &hcan1)
    {
        return s_can1_dev;
    }
    if (handle == &hcan2)
    {
        return s_can2_dev;
    }

    return NULL;
}

static int CANServiceInit(void)
{
    int ret;

    if (s_can_service_inited)
    {
        return 0;
    }

    memset(&hcan1, 0, sizeof(hcan1));
    memset(&hcan2, 0, sizeof(hcan2));
    s_can1_dev = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_primary));
    s_can2_dev = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_secondary));

    if (!device_is_ready(s_can1_dev) || !device_is_ready(s_can2_dev))
    {
        LOG_ERR("CAN devices are not ready");
        return -ENODEV;
    }

    ret = can_start(s_can1_dev);
    if ((ret != 0) && (ret != -EALREADY))
    {
        LOG_ERR("failed to start can1: %d", ret);
        return ret;
    }

    ret = can_start(s_can2_dev);
    if ((ret != 0) && (ret != -EALREADY))
    {
        LOG_ERR("failed to start can2: %d", ret);
        return ret;
    }

    s_can_service_inited = true;
    LOG_INF("CAN service initialized");
    return 0;
}

static int BSPCANSendFrame(CANInstance *instance, const uint8_t *data, uint8_t length, k_timeout_t timeout)
{
    struct can_frame frame;
    uint8_t send_len;
    int ret;

    const struct device *dev = NULL;

    if ((instance == NULL) || (instance->can_handle == NULL) || (data == NULL) || (length == 0U) || (length > 8U))
    {
        return -EINVAL;
    }

    dev = BSPCANResolveDevice(instance->can_handle);
    if (dev == NULL)
    {
        return -ENODEV;
    }

    memset(&frame, 0, sizeof(frame));
    send_len = instance->txconf.DLC;
    if ((send_len == 0U) || (send_len > 8U))
    {
        send_len = length;
    }
    if (send_len > length)
    {
        send_len = length;
    }

    frame.id = instance->txconf.StdId;
    frame.flags = 0U;
    frame.dlc = send_len;
    memcpy(frame.data, data, send_len);

    ret = can_send(dev, &frame, timeout, NULL, NULL);
    if (ret != 0)
    {
        LOG_WRN("CAN send failed tx=0x%03x ret=%d", (unsigned int)frame.id, ret);
    }

    return ret;
}

CANInstance *CANRegister(CAN_Init_Config_s *config)
{
    struct can_filter filter;
    CANInstance *instance;
    int filter_id;

    const struct device *dev = NULL;

    if ((config == NULL) || (config->can_handle == NULL))
    {
        return NULL;
    }

    if (CANServiceInit() != 0)
    {
        return NULL;
    }

    if (s_can_instance_count >= CAN_MX_REGISTER_CNT)
    {
        LOG_ERR("CAN instance count exceeded");
        return NULL;
    }

    instance = (CANInstance *)malloc(sizeof(CANInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    dev = BSPCANResolveDevice(config->can_handle);
    if (dev == NULL)
    {
        free(instance);
        return NULL;
    }

    memset(instance, 0, sizeof(CANInstance));
    instance->can_handle = config->can_handle;
    instance->tx_id = config->tx_id;
    instance->rx_id = config->rx_id;
    instance->can_module_callback = config->can_module_callback;
    instance->id = config->id;
    instance->txconf.StdId = config->tx_id;
    instance->txconf.IDE = CAN_ID_STD;
    instance->txconf.RTR = CAN_RTR_DATA;
    instance->txconf.DLC = 8U;

    memset(&filter, 0, sizeof(filter));
    filter.id = config->rx_id;
    filter.mask = CAN_STD_ID_MASK;
    filter.flags = 0U;
    filter_id = can_add_rx_filter(dev, BSPCANRxCallback, instance, &filter);
    if (filter_id < 0)
    {
        LOG_ERR("failed to add CAN rx filter 0x%03x: %d", (unsigned int)config->rx_id, filter_id);
        free(instance);
        return NULL;
    }

    instance->rx_filter_id = filter_id;
    s_can_instances[s_can_instance_count++] = instance;
    return instance;
}

void CANSetDLC(CANInstance *_instance, uint8_t length)
{
    if ((_instance == NULL) || (length == 0U) || (length > 8U))
    {
        LOG_ERR("invalid CAN DLC: %u", (unsigned int)length);
        return;
    }

    _instance->txconf.DLC = length;
}

uint8_t CANTransmit(CANInstance *_instance, float timeout)
{
    uint32_t timeout_ms;

    if (_instance == NULL)
    {
        return 0U;
    }

    timeout_ms = timeout <= 0.0f ? 0U : (uint32_t)(timeout + 0.999f);
    return BSPCANSendFrame(_instance, _instance->tx_buff, _instance->txconf.DLC, K_MSEC(timeout_ms)) == 0 ? 1U : 0U;
}

uint8_t CANTransmitTry(CANInstance *_instance)
{
    if (_instance == NULL)
    {
        return 0U;
    }

    return BSPCANSendFrame(_instance, _instance->tx_buff, _instance->txconf.DLC, K_NO_WAIT) == 0 ? 1U : 0U;
}

uint8_t CANTransmitEx(CANInstance *_instance, const uint8_t *data, uint8_t length)
{
    if (_instance == NULL)
    {
        return 0U;
    }

    return BSPCANSendFrame(_instance, data, length, K_NO_WAIT) == 0 ? 1U : 0U;
}
