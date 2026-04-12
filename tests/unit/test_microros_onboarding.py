from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_microros_module_and_service_scaffold_exist() -> None:
    expected = [
        "modules/lib/libmicroros/zephyr/module.yml",
        "modules/lib/libmicroros/patch_microros_sources.sh",
        "modules/lib/libmicroros/sanitize_microros_env.sh",
        "modules/lib/libmicroros/patches/embedded-no-exit.patch",
        "modules/lib/libmicroros/patches/rcutils-zephyr4.patch",
        "modules/lib/libmicroros/templates/rcutils_time_unix_zephyr.c",
        "modules/rscf/services/rscf_microros_service.c",
        "modules/rscf/services/rscf_microros_service.h",
        "include/rscf/rscf_microros_service.h",
        "scripts/microros/run_agent_usb.sh",
    ]

    for relpath in expected:
        assert (REPO_ROOT / relpath).is_file(), relpath


def test_app_cmake_and_prj_conf_enable_microros_path() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")
    microros_module_cmake = (REPO_ROOT / "modules" / "lib" / "libmicroros" / "CMakeLists.txt").read_text(
        encoding="utf-8"
    )
    prj_conf = (REPO_ROOT / "app" / "prj.conf").read_text(encoding="utf-8")

    assert "libmicroros" in app_cmake
    assert "rscf_microros_service.c" in app_cmake
    assert "target_include_directories(microros INTERFACE ${MICROROS_DIR}/include)" in microros_module_cmake
    assert "get_package_names" not in microros_module_cmake
    assert "execute_process(" not in microros_module_cmake

    assert "CONFIG_RSCF_ROS_BACKEND_MICROROS=y" in prj_conf
    assert "CONFIG_RSCF_MICROROS_SERVICE=y" in prj_conf
    assert "CONFIG_RSCF_MICROROS_SIM_MODE=y" in prj_conf
    assert "CONFIG_RSCF_COMM_SERVICE=n" in prj_conf


def test_bootstrap_installs_microros_host_tools_into_repo_venv() -> None:
    bootstrap = (REPO_ROOT / "scripts" / "bootstrap" / "bootstrap.sh").read_text(encoding="utf-8")

    assert "python3 -m pip install west colcon-common-extensions vcstool" in bootstrap


def test_kconfig_and_profile_expose_microros_backend() -> None:
    modules_kconfig = (REPO_ROOT / "modules" / "rscf" / "Kconfig").read_text(encoding="utf-8")
    ros_bridge_c = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_ros_bridge.c").read_text(encoding="utf-8")

    assert "config RSCF_ROS_BACKEND_MICROROS" in modules_kconfig
    assert "config RSCF_MICROROS_SERVICE" in modules_kconfig
    assert "config RSCF_MICROROS_SIM_MODE" in modules_kconfig
    assert "config RSCF_MICROROS_ODOM_RATE_HZ" in modules_kconfig
    assert "config RSCF_MICROROS_CMD_RATE_HZ" in modules_kconfig

    assert "RSCFMicroRosServiceInit" in ros_bridge_c
    assert "RSCFMicroRosServiceTick" in ros_bridge_c
    assert "RSCF_ROS_BACKEND_MICROROS" in ros_bridge_c
    assert "micro_ros_usb" in ros_bridge_c


def test_microros_service_targets_200hz_usb_sim_bridge() -> None:
    service_c = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_microros_service.c").read_text(
        encoding="utf-8"
    )
    transport_c = (
        REPO_ROOT / "modules" / "lib" / "libmicroros" / "microros_transports" / "serial-usb" / "microros_transports.c"
    ).read_text(encoding="utf-8")
    run_agent_script = (REPO_ROOT / "scripts" / "microros" / "run_agent_usb.sh").read_text(encoding="utf-8")

    assert "geometry_msgs/msg/twist.h" in service_c
    assert "nav_msgs/msg/odometry.h" in service_c
    assert "sensor_msgs/msg/imu.h" in service_c
    assert "std_msgs/msg/float32.h" in service_c
    assert "std_msgs/msg/u_int32.h" in service_c
    assert "rcutils_set_default_allocator" in service_c
    assert "rmw_uros_sync_session" in service_c
    assert "rmw_uros_epoch_synchronized" in service_c
    assert "rmw_uros_set_custom_transport" in service_c
    assert "rclc_publisher_init_default(\n        &s_microros.odom_pub" in service_c
    assert "RSCF_MICROROS_ODOM_PERIOD_MS" in service_c
    assert "RSCF_MICROROS_CMD_PERIOD_MS" in service_c
    assert "RSCF_MICROROS_STATUS_PERIOD_MS" in service_c
    assert "RSCFImuUartTryGetLatest" in service_c
    assert "RobotCmd" in service_c
    assert "uart_irq_rx_enable(params->uart_dev);" in transport_c
    assert "uart_poll_out(params->uart_dev, buf[i]);" in transport_c
    assert "find /dev/serial/by-id" in run_agent_script
    assert "ttyACM" in run_agent_script


def test_libmicroros_build_flow_has_repeatable_rcutils_patch_step() -> None:
    libmicroros_mk = (REPO_ROOT / "modules" / "lib" / "libmicroros" / "libmicroros.mk").read_text(
        encoding="utf-8"
    )
    colcon_meta = (REPO_ROOT / "modules" / "lib" / "libmicroros" / "colcon.meta").read_text(encoding="utf-8")
    patch_script = (
        REPO_ROOT / "modules" / "lib" / "libmicroros" / "patch_microros_sources.sh"
    ).read_text(encoding="utf-8")
    env_script = (
        REPO_ROOT / "modules" / "lib" / "libmicroros" / "sanitize_microros_env.sh"
    ).read_text(encoding="utf-8")
    rcutils_patch = (
        REPO_ROOT / "modules" / "lib" / "libmicroros" / "patches" / "rcutils-zephyr4.patch"
    ).read_text(encoding="utf-8")
    embedded_patch = (
        REPO_ROOT / "modules" / "lib" / "libmicroros" / "patches" / "embedded-no-exit.patch"
    ).read_text(encoding="utf-8")
    time_template = (
        REPO_ROOT / "modules" / "lib" / "libmicroros" / "templates" / "rcutils_time_unix_zephyr.c"
    ).read_text(encoding="utf-8")

    assert "patch_vendor_sources" in libmicroros_mk
    assert "patch_microros_sources.sh" in libmicroros_mk
    assert "sanitize_microros_env.sh" in libmicroros_mk
    assert "rosidl_dynamic_typesupport/COLCON_IGNORE" in libmicroros_mk
    assert 'link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rosidl_dynamic_typesupport"' not in libmicroros_mk
    assert 'link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rosidl_core"' not in libmicroros_mk
    assert "ROSIDL_TYPESUPPORT_SINGLE_TYPESUPPORT=ON" in colcon_meta
    assert "MICROROS_DEV_PACKAGES" in libmicroros_mk
    assert "--packages-up-to $(MICROROS_DEV_PACKAGES)" in libmicroros_mk
    assert "COLCON_PYTHON_EXECUTABLE=$(MICROROS_PYTHON)" in libmicroros_mk
    assert "PYTHONNOUSERSITE=1" in libmicroros_mk
    assert "Python3_FIND_VIRTUALENV=STANDARD" in libmicroros_mk
    assert "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON" in libmicroros_mk
    assert "--cmake-clean-cache" in libmicroros_mk
    assert "CMAKE_IGNORE_PREFIX_PATH=/opt/ros/humble" in libmicroros_mk
    assert 'rm -rf $(COMPONENT_PATH)/include' in libmicroros_mk
    assert 'if [ -d "$$pkg_dir/$$pkg_name" ]; then' in libmicroros_mk
    assert 'cp -R "$$pkg_dir/$$pkg_name"/. "$$pkg_dir"/' in libmicroros_mk
    assert 'rm -rf "$$pkg_dir/$$pkg_name"' in libmicroros_mk
    assert "VIRTUAL_ENV" in env_script
    assert '"$VIRTUAL_ENV"|"$VIRTUAL_ENV"/*' not in env_script
    assert "MICROROS_STRIP_PREFIXES" in env_script
    assert "clean_var AMENT_PREFIX_PATH" in env_script
    assert "clean_var CMAKE_PREFIX_PATH" in env_script
    assert "rcutils-zephyr4.patch" in patch_script
    assert "embedded-no-exit.patch" in patch_script
    assert 'ensure_local_copy "${src_root}/rosidl"' in patch_script
    assert 'ensure_local_copy "${src_root}/rcl"' in patch_script
    assert "apply_tree_patch_if_needed" in patch_script
    assert "fix_embedded_exit_sources" in patch_script
    assert "s/exit(-1);/abort();/g" in patch_script
    assert "disable_optional_packages" in patch_script
    assert "rosidl_core/COLCON_IGNORE" in patch_script
    assert "sync_time_unix_template" in patch_script
    assert "rcutils_time_unix_zephyr.c" in patch_script
    assert "RCUTILS_MS_TO_NS((int64_t)k_uptime_get())" in time_template
    assert "#include <zephyr/kernel.h>" in time_template
    assert "IS_STREAM_A_TTY(stream) (false)" in rcutils_patch
    assert "#include <strings.h>" in rcutils_patch
    assert "abort();" in embedded_patch
    assert "string_functions.c" in embedded_patch
    assert "u16string_functions.c" in embedded_patch
    assert "rmw_implementation_identifier_check.c" in embedded_patch
