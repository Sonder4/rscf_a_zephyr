from pathlib import Path
import subprocess
import tempfile


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_codegen_emits_transport_override_and_tx_hooks():
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "generated"
        script = REPO_ROOT / "scripts" / "codegen" / "rscf_mcu_comm_codegen.py"

        subprocess.run(
            [
                "python3",
                str(script),
                "--device",
                "MCU_ONE",
                "--output-dir",
                str(output_dir),
            ],
            check=True,
            cwd=REPO_ROOT,
        )

        robot_interface_h = (output_dir / "app" / "robot_interface.h").read_text(encoding="utf-8")
        robot_interface_c = (output_dir / "app" / "robot_interface.c").read_text(encoding="utf-8")

        assert "void RobotInterfaceSetTransport(Transport_interface_t* transport);" in robot_interface_h
        assert "void RobotInterfaceTxPeriodicHook(void);" in robot_interface_h
        assert "void RobotInterfaceTxEventHook(uint32_t tx_events);" in robot_interface_h
        assert "RobotInterfaceTxPeriodicHook();" in robot_interface_c
        assert "RobotInterfaceTxEventHook(tx_events);" in robot_interface_c


def test_codegen_emits_real_usb_cdc_acm_transport():
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "generated"
        script = REPO_ROOT / "scripts" / "codegen" / "rscf_mcu_comm_codegen.py"

        subprocess.run(
            [
                "python3",
                str(script),
                "--device",
                "MCU_ONE",
                "--output-dir",
                str(output_dir),
            ],
            check=True,
            cwd=REPO_ROOT,
        )

        transport_usb_c = (output_dir / "transports" / "transport_usb.c").read_text(encoding="utf-8")

        assert "DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart)" in transport_usb_c
        assert "usb_enable(NULL)" in transport_usb_c
        assert "UART_LINE_CTRL_DCD" not in transport_usb_c
        assert "UART_LINE_CTRL_DSR" not in transport_usb_c
        assert "-ENOTSUP" not in transport_usb_c


def test_codegen_defaults_zephyr_transport_to_usb():
    with tempfile.TemporaryDirectory() as temp_dir:
        output_dir = Path(temp_dir) / "generated"
        script = REPO_ROOT / "scripts" / "codegen" / "rscf_mcu_comm_codegen.py"

        subprocess.run(
            [
                "python3",
                str(script),
                "--device",
                "MCU_ONE",
                "--output-dir",
                str(output_dir),
            ],
            check=True,
            cwd=REPO_ROOT,
        )

        transport_interface_h = (output_dir / "transports" / "transport_interface.h").read_text(
            encoding="utf-8"
        )

        assert "#define MCU_COMM_DEFAULT_TRANSPORT MCU_COMM_TRANSPORT_USB" in transport_interface_h
