#include "rscf_microros_service.h"

#include <errno.h>
#include <math.h>
#include <string.h>

#include <arm_math.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <geometry_msgs/msg/twist.h>
#include <nav_msgs/msg/odometry.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <rcutils/allocator.h>
#include <rmw_microros/custom_transport.h>
#include <rmw_microros/ping.h>
#include <rmw_microros/time_sync.h>
#include <rosidl_runtime_c/string_functions.h>
#include <sensor_msgs/msg/imu.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/u_int32.h>

#include <rscf/rscf_chassis_service.h>

#include "data_def.h"
#include "microros_transports.h"
#include "robot_cmd.h"
#include "rscf_imu_uart.h"

LOG_MODULE_REGISTER(rscf_microros_service, LOG_LEVEL_INF);

#define RSCF_MICROROS_RETRY_PERIOD_MS 250U
#define RSCF_MICROROS_AGENT_PING_TIMEOUT_MS 10U
#define RSCF_MICROROS_CMD_TIMEOUT_MS 100U
#define RSCF_MICROROS_STATUS_PERIOD_MS (1000U / CONFIG_RSCF_MICROROS_STATUS_RATE_HZ)
#define RSCF_MICROROS_ODOM_PERIOD_MS (1000U / CONFIG_RSCF_MICROROS_ODOM_RATE_HZ)
#define RSCF_MICROROS_CMD_PERIOD_MS (1000U / CONFIG_RSCF_MICROROS_CMD_RATE_HZ)
#define RSCF_MICROROS_AGENT_HEALTH_PERIOD_MS 1000U
#define RSCF_MICROROS_TIME_SYNC_PERIOD_MS 3000U
#define RSCF_MICROROS_TIME_SYNC_TIMEOUT_MS 200U
#define RSCF_MICROROS_PI 3.14159265358979323846f
#define RSCF_MICROROS_DEG_TO_RAD (RSCF_MICROROS_PI / 180.0f)
#define RSCF_MICROROS_MAX_SIM_YAW_RADPS RSCF_MICROROS_PI
#define RSCF_MICROROS_HEADING_KP 3.0f

typedef struct
{
    bool ready;
    bool connected;
    bool entities_ready;
    bool transport_ready;
    bool allocator_ready;
    bool epoch_synced;
    uint32_t last_retry_tick_ms;
    uint32_t last_fast_pub_tick_ms;
    uint32_t last_status_pub_tick_ms;
    uint32_t last_cmd_tick_ms;
    uint32_t last_health_tick_ms;
    uint32_t last_time_sync_tick_ms;
    uint32_t status_seq;
    float target_vx_mps;
    float target_vy_mps;
    float target_wz_radps;
    float target_heading_deg;
    bool heading_target_valid;
    float sim_x_m;
    float sim_y_m;
    float sim_yaw_rad;
    float sim_world_vx_mps;
    float sim_world_vy_mps;
    float sim_wz_radps;
    geometry_msgs__msg__Twist cmd_vel_msg;
    std_msgs__msg__Float32 heading_msg;
    nav_msgs__msg__Odometry odom_msg;
    sensor_msgs__msg__Imu imu_msg;
    std_msgs__msg__UInt32 status_msg;
    rcl_allocator_t allocator;
    rclc_support_t support;
    rcl_node_t node;
    rcl_publisher_t odom_pub;
    rcl_publisher_t imu_pub;
    rcl_publisher_t status_pub;
    rcl_subscription_t cmd_vel_sub;
    rcl_subscription_t heading_sub;
    rclc_executor_t executor;
} RSCFMicroRosRuntime_s;

static RSCFMicroRosRuntime_s s_microros = {
    .node = {0},
    .odom_pub = {0},
    .imu_pub = {0},
    .status_pub = {0},
    .cmd_vel_sub = {0},
    .heading_sub = {0},
    .executor = {0},
};

static void *RSCFMicroRosAllocate(size_t size, void *state)
{
    ARG_UNUSED(state);
    return k_malloc(size);
}

static void RSCFMicroRosDeallocate(void *pointer, void *state)
{
    ARG_UNUSED(state);
    k_free(pointer);
}

static void *RSCFMicroRosReallocate(void *pointer, size_t size, void *state)
{
    ARG_UNUSED(state);
    return k_realloc(pointer, size);
}

static void *RSCFMicroRosZeroAllocate(size_t number_of_elements, size_t size_of_element, void *state)
{
    ARG_UNUSED(state);
    return k_calloc(number_of_elements, size_of_element);
}

static bool RSCFMicroRosSetupAllocator(void)
{
    rcutils_allocator_t allocator = rcutils_get_zero_initialized_allocator();

    if (s_microros.allocator_ready)
    {
        return true;
    }

    allocator.allocate = RSCFMicroRosAllocate;
    allocator.deallocate = RSCFMicroRosDeallocate;
    allocator.reallocate = RSCFMicroRosReallocate;
    allocator.zero_allocate = RSCFMicroRosZeroAllocate;

    if (!rcutils_set_default_allocator(&allocator))
    {
        return false;
    }

    s_microros.allocator_ready = true;
    return true;
}

static uint64_t RSCFMicroRosGetNowNs(void)
{
    int64_t epoch_ns = 0;

    if (s_microros.epoch_synced && rmw_uros_epoch_synchronized())
    {
        epoch_ns = rmw_uros_epoch_nanos();
        if (epoch_ns > 0)
        {
            return (uint64_t)epoch_ns;
        }
    }

    return k_ticks_to_ns_floor64(k_uptime_ticks());
}

static bool RSCFMicroRosCheckRclRet(const char *tag, rcl_ret_t rc)
{
    if (rc == RCL_RET_OK)
    {
        return true;
    }

    LOG_WRN("%s failed: %d", tag, (int)rc);
    return false;
}

static void RSCFMicroRosTrySyncTime(void)
{
    rmw_ret_t rc = rmw_uros_sync_session(RSCF_MICROROS_TIME_SYNC_TIMEOUT_MS);

    if ((rc == RMW_RET_OK) && rmw_uros_epoch_synchronized())
    {
        if (!s_microros.epoch_synced)
        {
            LOG_INF("micro-ROS epoch synchronized");
        }
        s_microros.epoch_synced = true;
        return;
    }

    if (s_microros.epoch_synced)
    {
        LOG_WRN("micro-ROS epoch sync lost: %d", (int)rc);
    }
    s_microros.epoch_synced = false;
}

static float RSCFMicroRosClampF(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

static float RSCFMicroRosWrapRad(float angle_rad)
{
    while (angle_rad > RSCF_MICROROS_PI)
    {
        angle_rad -= 2.0f * RSCF_MICROROS_PI;
    }

    while (angle_rad < -RSCF_MICROROS_PI)
    {
        angle_rad += 2.0f * RSCF_MICROROS_PI;
    }

    return angle_rad;
}

static void RSCFMicroRosFillStamp(builtin_interfaces__msg__Time *stamp)
{
    uint64_t now_ns = RSCFMicroRosGetNowNs();

    if (stamp == NULL)
    {
        return;
    }

    stamp->sec = (int32_t)(now_ns / 1000000000ULL);
    stamp->nanosec = (uint32_t)(now_ns % 1000000000ULL);
}

static void RSCFMicroRosFillQuaternion(float roll_rad,
                                       float pitch_rad,
                                       float yaw_rad,
                                       geometry_msgs__msg__Quaternion *quat)
{
    float cr;
    float sr;
    float cp;
    float sp;
    float cy;
    float sy;

    if (quat == NULL)
    {
        return;
    }

    cr = arm_cos_f32(roll_rad * 0.5f);
    sr = arm_sin_f32(roll_rad * 0.5f);
    cp = arm_cos_f32(pitch_rad * 0.5f);
    sp = arm_sin_f32(pitch_rad * 0.5f);
    cy = arm_cos_f32(yaw_rad * 0.5f);
    sy = arm_sin_f32(yaw_rad * 0.5f);

    quat->w = cr * cp * cy + sr * sp * sy;
    quat->x = sr * cp * cy - cr * sp * sy;
    quat->y = cr * sp * cy + sr * cp * sy;
    quat->z = cr * cp * sy - sr * sp * cy;
}

static void RSCFMicroRosApplyCmdVel(const geometry_msgs__msg__Twist *msg)
{
    if (msg == NULL)
    {
        return;
    }

    s_microros.target_vx_mps = (float)msg->linear.x;
    s_microros.target_vy_mps = (float)msg->linear.y;
    s_microros.target_wz_radps = (float)msg->angular.z;
    s_microros.last_cmd_tick_ms = k_uptime_get_32();

    RSCFChassisServiceSetControlMode(CONTROL_PLANE_CHASSIS_CTRL_MODE_CMD_VEL);
    RSCFChassisServiceSetVelocityFrame(CONTROL_PLANE_VELOCITY_FRAME_BODY);
    RSCFChassisServiceSetTarget(s_microros.target_vx_mps,
                                s_microros.target_vy_mps,
                                s_microros.target_wz_radps * (180.0f / RSCF_MICROROS_PI));
}

static void RSCFMicroRosCmdVelCallback(const void *msgin)
{
    RSCFMicroRosApplyCmdVel((const geometry_msgs__msg__Twist *)msgin);
}

static void RSCFMicroRosHeadingCallback(const void *msgin)
{
    const std_msgs__msg__Float32 *msg = (const std_msgs__msg__Float32 *)msgin;

    if (msg == NULL)
    {
        return;
    }

    s_microros.target_heading_deg = msg->data;
    s_microros.heading_target_valid = true;
    RSCFChassisServiceSetHeadingMode(CONTROL_PLANE_HEADING_MODE_TARGET_HEADING);
    RSCFChassisServiceSetHeadingTarget(msg->data);
}

static uint32_t RSCFMicroRosPackStatus(bool imu_ready, bool odom_ready)
{
    uint32_t status_word = 0U;
    uint32_t legacy_status = 0U;

    legacy_status |= (uint32_t)g_system_status.status_bytes[0];
    legacy_status |= ((uint32_t)g_system_status.status_bytes[1] << 8);
    legacy_status |= ((uint32_t)g_system_status.status_bytes[2] << 16);

    status_word = legacy_status;
    if (s_microros.ready)
    {
        status_word |= (1UL << 24);
    }

    if (s_microros.connected)
    {
        status_word |= (1UL << 25);
    }

#if defined(CONFIG_RSCF_MICROROS_SIM_MODE)
    status_word |= (1UL << 26);
#endif

    if (imu_ready)
    {
        status_word |= (1UL << 27);
    }

    if (odom_ready)
    {
        status_word |= (1UL << 28);
    }

    status_word |= ((uint32_t)RobotCmdGetArmState() & 0x03U) << 29;
    return status_word;
}

static void RSCFMicroRosAdvanceSimulation(uint32_t dt_ms)
{
    float dt_s = ((float)dt_ms) * 0.001f;
    float body_vx = s_microros.target_vx_mps;
    float body_vy = s_microros.target_vy_mps;
    float wz = s_microros.target_wz_radps;
    uint32_t now_tick = k_uptime_get_32();
    float world_vx;
    float world_vy;

    if ((uint32_t)(now_tick - s_microros.last_cmd_tick_ms) > RSCF_MICROROS_CMD_TIMEOUT_MS)
    {
        body_vx = 0.0f;
        body_vy = 0.0f;
        wz = 0.0f;
    }

    if (s_microros.heading_target_valid && (fabsf(wz) < 1e-3f))
    {
        float target_heading_rad = s_microros.target_heading_deg * RSCF_MICROROS_DEG_TO_RAD;
        float error_rad = RSCFMicroRosWrapRad(target_heading_rad - s_microros.sim_yaw_rad);

        wz = RSCFMicroRosClampF(error_rad * RSCF_MICROROS_HEADING_KP,
                                -RSCF_MICROROS_MAX_SIM_YAW_RADPS,
                                RSCF_MICROROS_MAX_SIM_YAW_RADPS);
    }

    s_microros.sim_yaw_rad = RSCFMicroRosWrapRad(s_microros.sim_yaw_rad + wz * dt_s);
    world_vx = body_vx * arm_cos_f32(s_microros.sim_yaw_rad) - body_vy * arm_sin_f32(s_microros.sim_yaw_rad);
    world_vy = body_vx * arm_sin_f32(s_microros.sim_yaw_rad) + body_vy * arm_cos_f32(s_microros.sim_yaw_rad);

    s_microros.sim_x_m += world_vx * dt_s;
    s_microros.sim_y_m += world_vy * dt_s;
    s_microros.sim_world_vx_mps = world_vx;
    s_microros.sim_world_vy_mps = world_vy;
    s_microros.sim_wz_radps = wz;
}

static void RSCFMicroRosPublishOdom(bool *odom_ready_out)
{
    bool odom_ready = false;
    float x_m = 0.0f;
    float y_m = 0.0f;
    float yaw_rad = 0.0f;
    float vx_mps = 0.0f;
    float vy_mps = 0.0f;
    float wz_radps = 0.0f;

#if defined(CONFIG_RSCF_MICROROS_SIM_MODE)
    x_m = s_microros.sim_x_m;
    y_m = s_microros.sim_y_m;
    yaw_rad = s_microros.sim_yaw_rad;
    vx_mps = s_microros.sim_world_vx_mps;
    vy_mps = s_microros.sim_world_vy_mps;
    wz_radps = s_microros.sim_wz_radps;
    odom_ready = true;
#else
    odom_ready = RSCFChassisServiceGetOdom(&x_m, &y_m, &yaw_rad, &vx_mps, &vy_mps, &wz_radps);
#endif

    if (!odom_ready)
    {
        if (odom_ready_out != NULL)
        {
            *odom_ready_out = false;
        }
        return;
    }

    RSCFMicroRosFillStamp(&s_microros.odom_msg.header.stamp);
    s_microros.odom_msg.pose.pose.position.x = x_m;
    s_microros.odom_msg.pose.pose.position.y = y_m;
    s_microros.odom_msg.pose.pose.position.z = 0.0;
    s_microros.odom_msg.twist.twist.linear.x = vx_mps;
    s_microros.odom_msg.twist.twist.linear.y = vy_mps;
    s_microros.odom_msg.twist.twist.angular.z = wz_radps;
    RSCFMicroRosFillQuaternion(0.0f, 0.0f, yaw_rad, &s_microros.odom_msg.pose.pose.orientation);
    if (!RSCFMicroRosCheckRclRet("publish odom", rcl_publish(&s_microros.odom_pub, &s_microros.odom_msg, NULL)))
    {
        s_microros.connected = false;
    }

    if (odom_ready_out != NULL)
    {
        *odom_ready_out = true;
    }
}

static void RSCFMicroRosPublishImu(bool *imu_ready_out)
{
    bool imu_ready = false;
    float roll_deg = 0.0f;
    float pitch_deg = 0.0f;
    float yaw_deg = 0.0f;
    float gyro_z_dps = 0.0f;
    float acc_x = 0.0f;
    float acc_y = 0.0f;
    float acc_z = 0.0f;

#if defined(CONFIG_RSCF_MICROROS_SIM_MODE)
    yaw_deg = s_microros.sim_yaw_rad * (180.0f / RSCF_MICROROS_PI);
    gyro_z_dps = s_microros.sim_wz_radps * (180.0f / RSCF_MICROROS_PI);
    imu_ready = true;
#else
    struct rscf_saber_topic_data latest = {0};

    if (RSCFImuUartTryGetLatest(&latest))
    {
        roll_deg = latest.euler.roll;
        pitch_deg = latest.euler.pitch;
        yaw_deg = latest.euler.yaw;
        gyro_z_dps = latest.kalman_gyro.z;
        acc_x = latest.kalman_acc.x;
        acc_y = latest.kalman_acc.y;
        acc_z = latest.kalman_acc.z;
        imu_ready = true;
    }
    else
    {
        imu_ready = RSCFChassisServiceGetImuData(&roll_deg,
                                                 &pitch_deg,
                                                 &yaw_deg,
                                                 &gyro_z_dps,
                                                 &acc_x,
                                                 &acc_y,
                                                 &acc_z);
    }
#endif

    if (!imu_ready)
    {
        if (imu_ready_out != NULL)
        {
            *imu_ready_out = false;
        }
        return;
    }

    RSCFMicroRosFillStamp(&s_microros.imu_msg.header.stamp);
    s_microros.imu_msg.angular_velocity.z = gyro_z_dps * RSCF_MICROROS_DEG_TO_RAD;
    s_microros.imu_msg.linear_acceleration.x = acc_x;
    s_microros.imu_msg.linear_acceleration.y = acc_y;
    s_microros.imu_msg.linear_acceleration.z = acc_z;
    RSCFMicroRosFillQuaternion(roll_deg * RSCF_MICROROS_DEG_TO_RAD,
                               pitch_deg * RSCF_MICROROS_DEG_TO_RAD,
                               yaw_deg * RSCF_MICROROS_DEG_TO_RAD,
                               &s_microros.imu_msg.orientation);
    if (!RSCFMicroRosCheckRclRet("publish imu", rcl_publish(&s_microros.imu_pub, &s_microros.imu_msg, NULL)))
    {
        s_microros.connected = false;
    }

    if (imu_ready_out != NULL)
    {
        *imu_ready_out = true;
    }
}

static void RSCFMicroRosPublishStatus(bool imu_ready, bool odom_ready)
{
    s_microros.status_msg.data = RSCFMicroRosPackStatus(imu_ready, odom_ready);
    if (!RSCFMicroRosCheckRclRet("publish status", rcl_publish(&s_microros.status_pub, &s_microros.status_msg, NULL)))
    {
        s_microros.connected = false;
    }
}

static void RSCFMicroRosResetEntities(void)
{
    if (s_microros.entities_ready)
    {
        (void)RSCFMicroRosCheckRclRet("fini heading sub", rcl_subscription_fini(&s_microros.heading_sub, &s_microros.node));
        (void)RSCFMicroRosCheckRclRet("fini cmd_vel sub", rcl_subscription_fini(&s_microros.cmd_vel_sub, &s_microros.node));
        (void)RSCFMicroRosCheckRclRet("fini status pub", rcl_publisher_fini(&s_microros.status_pub, &s_microros.node));
        (void)RSCFMicroRosCheckRclRet("fini imu pub", rcl_publisher_fini(&s_microros.imu_pub, &s_microros.node));
        (void)RSCFMicroRosCheckRclRet("fini odom pub", rcl_publisher_fini(&s_microros.odom_pub, &s_microros.node));
        (void)RSCFMicroRosCheckRclRet("fini executor", rclc_executor_fini(&s_microros.executor));
        (void)RSCFMicroRosCheckRclRet("fini node", rcl_node_fini(&s_microros.node));
        (void)RSCFMicroRosCheckRclRet("fini support", rclc_support_fini(&s_microros.support));
        geometry_msgs__msg__Twist__fini(&s_microros.cmd_vel_msg);
        std_msgs__msg__Float32__fini(&s_microros.heading_msg);
        nav_msgs__msg__Odometry__fini(&s_microros.odom_msg);
        sensor_msgs__msg__Imu__fini(&s_microros.imu_msg);
        std_msgs__msg__UInt32__fini(&s_microros.status_msg);
    }

    memset(&s_microros.support, 0, sizeof(s_microros.support));
    memset(&s_microros.node, 0, sizeof(s_microros.node));
    memset(&s_microros.odom_pub, 0, sizeof(s_microros.odom_pub));
    memset(&s_microros.imu_pub, 0, sizeof(s_microros.imu_pub));
    memset(&s_microros.status_pub, 0, sizeof(s_microros.status_pub));
    memset(&s_microros.cmd_vel_sub, 0, sizeof(s_microros.cmd_vel_sub));
    memset(&s_microros.heading_sub, 0, sizeof(s_microros.heading_sub));
    memset(&s_microros.executor, 0, sizeof(s_microros.executor));
    s_microros.entities_ready = false;
    s_microros.connected = false;
    s_microros.epoch_synced = false;
}

static int RSCFMicroRosCreateEntities(void)
{
    rcl_ret_t rc;

    s_microros.allocator = rcl_get_default_allocator();
    rc = rmw_uros_set_custom_transport(MICRO_ROS_FRAMING_REQUIRED,
                                       (void *)&default_params,
                                       zephyr_transport_open,
                                       zephyr_transport_close,
                                       zephyr_transport_write,
                                       zephyr_transport_read);
    if (rc != RCL_RET_OK)
    {
        return -EIO;
    }

    rc = rclc_support_init(&s_microros.support, 0, NULL, &s_microros.allocator);
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_node_init_default(&s_microros.node, "rscf_a_node", "", &s_microros.support);
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_publisher_init_default(
        &s_microros.odom_pub,
        &s_microros.node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry),
        "/odom");
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_publisher_init_best_effort(
        &s_microros.imu_pub,
        &s_microros.node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
        "/imu/data");
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_publisher_init_default(
        &s_microros.status_pub,
        &s_microros.node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt32),
        "/rscf/system_status");
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_subscription_init_best_effort(
        &s_microros.cmd_vel_sub,
        &s_microros.node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel");
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_subscription_init_best_effort(
        &s_microros.heading_sub,
        &s_microros.node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
        "/rscf/heading_target_deg");
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    if (!geometry_msgs__msg__Twist__init(&s_microros.cmd_vel_msg) ||
        !std_msgs__msg__Float32__init(&s_microros.heading_msg) ||
        !nav_msgs__msg__Odometry__init(&s_microros.odom_msg) ||
        !sensor_msgs__msg__Imu__init(&s_microros.imu_msg) ||
        !std_msgs__msg__UInt32__init(&s_microros.status_msg))
    {
        RSCFMicroRosResetEntities();
        return -ENOMEM;
    }

    (void)rosidl_runtime_c__String__assign(&s_microros.odom_msg.header.frame_id, "odom");
    (void)rosidl_runtime_c__String__assign(&s_microros.odom_msg.child_frame_id, "base_link");
    (void)rosidl_runtime_c__String__assign(&s_microros.imu_msg.header.frame_id, "imu_link");

    rc = rclc_executor_init(&s_microros.executor, &s_microros.support.context, 2, &s_microros.allocator);
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_executor_add_subscription(
        &s_microros.executor, &s_microros.cmd_vel_sub, &s_microros.cmd_vel_msg, &RSCFMicroRosCmdVelCallback, ON_NEW_DATA);
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    rc = rclc_executor_add_subscription(
        &s_microros.executor, &s_microros.heading_sub, &s_microros.heading_msg, &RSCFMicroRosHeadingCallback, ON_NEW_DATA);
    if (rc != RCL_RET_OK)
    {
        RSCFMicroRosResetEntities();
        return -EIO;
    }

    s_microros.entities_ready = true;
    s_microros.connected = true;
    s_microros.last_fast_pub_tick_ms = k_uptime_get_32();
    s_microros.last_status_pub_tick_ms = s_microros.last_fast_pub_tick_ms;
    s_microros.last_health_tick_ms = s_microros.last_fast_pub_tick_ms;
    s_microros.last_time_sync_tick_ms = 0U;
    RSCFMicroRosTrySyncTime();
    s_microros.last_time_sync_tick_ms = k_uptime_get_32();
    LOG_INF("micro-ROS connected over USB");
    return 0;
}

int RSCFMicroRosServiceInit(void)
{
    rmw_ret_t transport_ret;

    memset(&s_microros, 0, sizeof(s_microros));
    if (!RSCFMicroRosSetupAllocator())
    {
        LOG_ERR("micro-ROS allocator setup failed");
        return -ENOMEM;
    }

    transport_ret = rmw_uros_set_custom_transport(MICRO_ROS_FRAMING_REQUIRED,
                                                  (void *)&default_params,
                                                  zephyr_transport_open,
                                                  zephyr_transport_close,
                                                  zephyr_transport_write,
                                                  zephyr_transport_read);
    if (transport_ret != RMW_RET_OK)
    {
        LOG_ERR("micro-ROS transport setup failed: %d", (int)transport_ret);
        return -EIO;
    }

    s_microros.ready = true;
    s_microros.transport_ready = true;
    s_microros.last_cmd_tick_ms = k_uptime_get_32();
    s_microros.last_retry_tick_ms = 0U;
    LOG_INF("micro-ROS service ready, waiting for agent");
    return 0;
}

void RSCFMicroRosServiceTick(void)
{
    uint32_t now_tick = k_uptime_get_32();

    if (!s_microros.ready)
    {
        return;
    }

    if (!s_microros.entities_ready)
    {
        if ((uint32_t)(now_tick - s_microros.last_retry_tick_ms) < RSCF_MICROROS_RETRY_PERIOD_MS)
        {
            return;
        }

        s_microros.last_retry_tick_ms = now_tick;
        if (rmw_uros_ping_agent(RSCF_MICROROS_AGENT_PING_TIMEOUT_MS, 1) != RMW_RET_OK)
        {
            return;
        }

        (void)RSCFMicroRosCreateEntities();
        return;
    }

    if ((uint32_t)(now_tick - s_microros.last_health_tick_ms) >= RSCF_MICROROS_AGENT_HEALTH_PERIOD_MS)
    {
        s_microros.last_health_tick_ms = now_tick;
        if (rmw_uros_ping_agent(RSCF_MICROROS_AGENT_PING_TIMEOUT_MS, 1) != RMW_RET_OK)
        {
            LOG_WRN("micro-ROS agent lost");
            RSCFMicroRosResetEntities();
            s_microros.last_retry_tick_ms = now_tick;
            return;
        }
        s_microros.connected = true;
    }

    if ((uint32_t)(now_tick - s_microros.last_time_sync_tick_ms) >= RSCF_MICROROS_TIME_SYNC_PERIOD_MS)
    {
        s_microros.last_time_sync_tick_ms = now_tick;
        RSCFMicroRosTrySyncTime();
    }

    if (!RSCFMicroRosCheckRclRet("executor spin", rclc_executor_spin_some(&s_microros.executor, RCL_MS_TO_NS(0))))
    {
        RSCFMicroRosResetEntities();
        s_microros.last_retry_tick_ms = now_tick;
        return;
    }

    while ((uint32_t)(now_tick - s_microros.last_fast_pub_tick_ms) >= RSCF_MICROROS_ODOM_PERIOD_MS)
    {
        bool imu_ready = false;
        bool odom_ready = false;

        s_microros.last_fast_pub_tick_ms += RSCF_MICROROS_ODOM_PERIOD_MS;
#if defined(CONFIG_RSCF_MICROROS_SIM_MODE)
        RSCFMicroRosAdvanceSimulation(RSCF_MICROROS_ODOM_PERIOD_MS);
#endif
        RSCFMicroRosPublishOdom(&odom_ready);
        RSCFMicroRosPublishImu(&imu_ready);
        if ((uint32_t)(now_tick - s_microros.last_status_pub_tick_ms) >= RSCF_MICROROS_STATUS_PERIOD_MS)
        {
            s_microros.last_status_pub_tick_ms = now_tick;
            RSCFMicroRosPublishStatus(imu_ready, odom_ready);
        }
    }
}

bool RSCFMicroRosServiceReady(void)
{
    return s_microros.ready;
}

bool RSCFMicroRosServiceConnected(void)
{
    return s_microros.connected;
}
