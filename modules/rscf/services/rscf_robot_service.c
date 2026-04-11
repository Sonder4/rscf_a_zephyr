#include <rscf/rscf_robot_service.h>

#include "robot.h"

static bool s_robot_service_ready;

int RSCFRobotServiceInit(void)
{
    if (s_robot_service_ready)
    {
        return 0;
    }

    RobotInit();
    s_robot_service_ready = true;
    return 0;
}

void RSCFRobotServiceTick(void)
{
    if (!s_robot_service_ready)
    {
        return;
    }

    RobotTask();
}

bool RSCFRobotServiceReady(void)
{
    return s_robot_service_ready;
}
