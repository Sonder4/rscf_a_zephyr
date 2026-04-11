from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_usb_board_overlay_and_prj_conf_present():
    overlay = REPO_ROOT / "app" / "boards" / "rscf_a_f427iih6.overlay"
    prj_conf = (REPO_ROOT / "app" / "prj.conf").read_text(encoding="utf-8")

    assert overlay.exists()

    overlay_text = overlay.read_text(encoding="utf-8")
    assert "compatible = \"zephyr,cdc-acm-uart\";" in overlay_text

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
