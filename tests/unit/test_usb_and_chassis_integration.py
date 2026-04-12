from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_usb_board_overlay_and_prj_conf_present():
    app_overlay = (REPO_ROOT / "app" / "app.overlay").read_text(encoding="utf-8")
    overlay = REPO_ROOT / "app" / "boards" / "rscf_a_f427iih6.overlay"
    prj_conf = (REPO_ROOT / "app" / "prj.conf").read_text(encoding="utf-8")

    assert 'compatible = "zephyr,cdc-acm-uart";' in app_overlay
    assert overlay.exists()

    assert "CONFIG_USB_DEVICE_STACK=y" in prj_conf
    assert "CONFIG_USB_CDC_ACM=y" in prj_conf
    assert "CONFIG_UART_LINE_CTRL=y" in prj_conf


def test_chassis_service_integrates_protocol_hooks():
    chassis_service = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_chassis_service.c").read_text(
        encoding="utf-8"
    )

    assert "void ChassisCtrlCallback(void *data)" in chassis_service
    assert "void RobotSwitchCallback(void *data)" in chassis_service
    assert "void SystemCmdCallback(void *data)" in chassis_service
    assert "void RobotInterfaceTxPeriodicHook(void)" in chassis_service
    assert "void RobotInterfaceTxEventHook(uint32_t tx_events)" in chassis_service
    assert "RSCFImuUartTryGetLatest" in chassis_service
    assert "RSCFChassisRegisterM3508Motors();" in chassis_service
    assert "DJIMotorInit(&cfg)" in chassis_service


def test_link_runtime_skeleton_is_added_to_build():
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")
    link_service_h = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_link_service.h").read_text(
        encoding="utf-8"
    )
    link_service_c = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_link_service.c").read_text(
        encoding="utf-8"
    )
    service_router_c = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_service_router.c").read_text(
        encoding="utf-8"
    )
    action_service_c = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_action_service.c").read_text(
        encoding="utf-8"
    )

    assert "../modules/rscf/services/rscf_link_service.c" in app_cmake
    assert "../modules/rscf/services/rscf_service_router.c" in app_cmake
    assert "../modules/rscf/services/rscf_action_service.c" in app_cmake
    assert "struct rscf_link_runtime" in link_service_h
    assert "struct ring_buf" in link_service_h
    assert "struct k_work_delayable" in link_service_h
    assert "struct k_msgq" in link_service_h
    assert "RSCFLinkServiceInit" in link_service_c
    assert "RSCFLinkServiceProcess" in link_service_c
    assert "RSCFLinkServiceGetRuntime" in link_service_c
    assert "RSCFServiceRouterInit" in service_router_c
    assert "RSCFServiceRouterDispatch" in service_router_c
    assert "RSCFActionServiceInit" in action_service_c
    assert "RSCFActionServiceProcess" in action_service_c
