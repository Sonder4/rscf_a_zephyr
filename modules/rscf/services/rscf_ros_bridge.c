#include <rscf/rscf_ros_bridge.h>

#include "rscf_comm_service.h"
#include "rscf_microros_service.h"
#include "robot_interface.h"

static bool s_ros_bridge_ready;
static RSCFRosBackend_e s_ros_backend = RSCF_ROS_BACKEND_MCU_COMM;

int RSCFRosBridgeInit(void)
{
#if defined(CONFIG_RSCF_ROS_BACKEND_MICROROS) && defined(CONFIG_RSCF_MICROROS_SERVICE)
    s_ros_backend = RSCF_ROS_BACKEND_MICROROS;
    s_ros_bridge_ready = (RSCFMicroRosServiceInit() == 0);
    return s_ros_bridge_ready ? 0 : -1;
#else
    s_ros_backend = RSCF_ROS_BACKEND_MCU_COMM;
    s_ros_bridge_ready = true;
    return 0;
#endif
}

void RSCFRosBridgeSpinOnce(void)
{
#if defined(CONFIG_RSCF_ROS_BACKEND_MICROROS) && defined(CONFIG_RSCF_MICROROS_SERVICE)
    if (!s_ros_bridge_ready)
    {
        return;
    }

    RSCFMicroRosServiceTick();
    return;
#else
    if ((!s_ros_bridge_ready) || (!RSCFCommServiceReady()))
    {
        return;
    }

    if (!RobotIsConnected())
    {
        return;
    }
#endif
}

bool RSCFRosBridgeReady(void)
{
#if defined(CONFIG_RSCF_ROS_BACKEND_MICROROS) && defined(CONFIG_RSCF_MICROROS_SERVICE)
    return s_ros_bridge_ready && RSCFMicroRosServiceReady();
#else
    return s_ros_bridge_ready;
#endif
}

RSCFRosBackend_e RSCFRosBridgeBackend(void)
{
    return s_ros_backend;
}

const char *RSCFRosBridgeTransportName(void)
{
#if defined(CONFIG_RSCF_ROS_BACKEND_MICROROS) && defined(CONFIG_RSCF_MICROROS_SERVICE)
    return "micro_ros_usb";
#else
    return "mcu_comm_usb";
#endif
}
