#include "robot.h"

#include <string.h>

#include "AD7190.h"
#include "HC05.h"
#include "IMU.h"
#include "main.h"
#include "master_process.h"
#include "mhall.h"
#include "oled.h"
#include "remote_control.h"
#include "robot_cmd.h"
#include "robot_interface.h"
#include "robot_task.h"
#include <rscf/rscf_ros_bridge.h>
#include "rscf_imu_uart.h"

static uint8_t s_robot_inited;
static uint32_t s_last_oled_tick;

void RobotInit(void)
{
    if (s_robot_inited != 0U)
    {
        return;
    }

    RobotCmdInit();
    SaberInit();
    (void)HC05Init(&huart2);
    (void)VisionInit(&huart3);
    (void)RemoteControlInit(&huart1);
    (void)AD7190Init();
    (void)OLEDInit();
    (void)RSCFRosBridgeInit();
    VisionSetFlag(COLOR_RED, VISION_MODE_AIM, SMALL_AMU_15);
    s_robot_inited = 1U;
}

void RobotTask(void)
{
    CapeEuler_Data_t euler = SaberGetEuler(g_saber_instance);
    uint32_t now_tick = k_uptime_get_32();

    if (s_robot_inited == 0U)
    {
        return;
    }

    RobotCmdTask();
    VisionSetAltitude(euler.yaw, euler.pitch, euler.roll);
    VisionSend();
    AD7190Task();
    MHallTask();
    RSCFRosBridgeSpinOnce();

    if ((uint32_t)(now_tick - s_last_oled_tick) >= ROBOT_OLED_PERIOD_MS)
    {
        s_last_oled_tick = now_tick;
        OLEDClear();
        OLEDShowString(0U, 0U, RobotIsConnected() ? "USB:ON" : "USB:WAIT");
        OLEDShowString(1U, 0U, HC05IsOnline() ? "BT:ON" : "BT:WAIT");
        OLEDShowString(2U, 0U, RemoteControlIsOnline() ? "RC:ON" : "RC:WAIT");
        OLEDShowString(3U, 0U, RSCFImuUartReady() ? "IMU:ON" : "IMU:WAIT");
    }

    OLEDTask();
}
