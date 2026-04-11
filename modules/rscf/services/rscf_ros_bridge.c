#include <rscf/rscf_ros_bridge.h>

#include "rscf_comm_service.h"
#include "robot_interface.h"

static bool s_ros_bridge_ready;
static RSCFRosBackend_e s_ros_backend = RSCF_ROS_BACKEND_MCU_COMM;

int RSCFRosBridgeInit(void)
{
    s_ros_backend = RSCF_ROS_BACKEND_MCU_COMM;
    s_ros_bridge_ready = true;
    return 0;
}

void RSCFRosBridgeSpinOnce(void)
{
    if ((!s_ros_bridge_ready) || (!RSCFCommServiceReady()))
    {
        return;
    }

    if (!RobotIsConnected())
    {
        return;
    }
}

bool RSCFRosBridgeReady(void)
{
    return s_ros_bridge_ready;
}

RSCFRosBackend_e RSCFRosBridgeBackend(void)
{
    return s_ros_backend;
}

const char *RSCFRosBridgeTransportName(void)
{
    return "mcu_comm_usb";
}
