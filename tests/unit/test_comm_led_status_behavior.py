from pathlib import Path
import subprocess
import textwrap


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_comm_led_status_behavior_is_wired() -> None:
    comm_template = (
        REPO_ROOT
        / "mcu_comm"
        / "protocol_generator"
        / "templates"
        / "zephyr"
        / "comm_manager_c.j2"
    ).read_text(encoding="utf-8")
    comm_service = (
        REPO_ROOT / "modules" / "rscf" / "services" / "rscf_comm_service.c"
    ).read_text(encoding="utf-8")
    led_header = (
        REPO_ROOT / "drivers" / "rscf" / "rscf_led_status.h"
    ).read_text(encoding="utf-8")
    led_driver = (
        REPO_ROOT / "drivers" / "rscf" / "rscf_led_status.c"
    ).read_text(encoding="utf-8")

    assert "Comm_OnValidRxFrame" in comm_template
    assert "Comm_OnValidRxFrame(pid);" in comm_template

    assert "RSCF_COMM_LED_FAST_HALF_MS 100U" in comm_service
    assert "RSCF_COMM_LED_SLOW_HALF_MS 500U" in comm_service
    assert "RSCFLedStatusSetBlinkHalfPeriod(RSCF_LED_STATUS_COMM" in comm_service

    assert "RSCFLedStatusSetBlinkHalfPeriod" in led_header
    assert "s_blink_half_period_ms" in led_driver


def test_comm_transport_adapter_and_compat_hooks_are_explicit() -> None:
    comm_template = (
        REPO_ROOT
        / "mcu_comm"
        / "protocol_generator"
        / "templates"
        / "zephyr"
        / "comm_manager_c.j2"
    ).read_text(encoding="utf-8")
    generated_comm = (
        REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE" / "core" / "comm_manager.c"
    ).read_text(encoding="utf-8")
    generated_robot_h = (
        REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE" / "app" / "robot_interface.h"
    ).read_text(encoding="utf-8")
    generated_robot_c = (
        REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE" / "app" / "robot_interface.c"
    ).read_text(encoding="utf-8")
    comm_service = (
        REPO_ROOT / "modules" / "rscf" / "services" / "rscf_comm_service.c"
    ).read_text(encoding="utf-8")
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "Comm_ResolveTransportOrDefault" in comm_template
    assert "Comm_OnTransportResolved" in comm_template
    assert "Comm_CompatMapEndpoint" in comm_template

    assert "Comm_ResolveTransportOrDefault" in generated_comm
    assert "Comm_OnTransportResolved" in generated_comm
    assert "Comm_CompatMapEndpoint" in generated_comm

    assert "RobotInterfaceResolveTransportOverride" in generated_robot_h
    assert "RobotInterfaceOnTransportReady" in generated_robot_h
    assert "RobotInterfaceFillCompatSystemStatus" in generated_robot_h
    assert "RobotInterfaceOnCompatSystemCmd" in generated_robot_h

    assert "RobotInterfaceResolveTransportOverride" in generated_robot_c
    assert "RobotInterfaceOnTransportReady" in generated_robot_c
    assert "RobotInterfaceFillCompatSystemStatus" in generated_robot_c
    assert "RobotInterfaceOnCompatSystemCmd" in generated_robot_c
    assert "RobotInterfaceCompatSystemCmdCallback" in generated_robot_c

    assert "RSCFCommServiceSelectTransport" in comm_service
    assert "RSCFCommServiceSelectAdapterRole" in comm_service
    assert "RSCFLinkUsbAdapterGetTransport" in comm_service
    assert "RSCFLinkUartAdapterGetTransport" in comm_service

    assert "../drivers/rscf/rscf_link_usb.c" in app_cmake
    assert "../drivers/rscf/rscf_link_uart.c" in app_cmake
    assert "../drivers/rscf/rscf_link_can.c" in app_cmake
    assert "../drivers/rscf/rscf_link_spi.c" in app_cmake


def test_transport_adapter_skeletons_use_unified_adapter_shape() -> None:
    adapter_header = (REPO_ROOT / "drivers" / "rscf" / "rscf_link_adapter.h").read_text(
        encoding="utf-8"
    )
    usb_adapter = (REPO_ROOT / "drivers" / "rscf" / "rscf_link_usb.c").read_text(encoding="utf-8")
    uart_adapter = (REPO_ROOT / "drivers" / "rscf" / "rscf_link_uart.c").read_text(encoding="utf-8")
    can_adapter = (REPO_ROOT / "drivers" / "rscf" / "rscf_link_can.c").read_text(encoding="utf-8")
    spi_adapter = (REPO_ROOT / "drivers" / "rscf" / "rscf_link_spi.c").read_text(encoding="utf-8")

    assert "struct rscf_link_adapter" in adapter_header
    assert "const struct rscf_link_adapter *RSCFLinkUsbAdapterGet(void);" in adapter_header
    assert "const struct rscf_link_adapter *RSCFLinkUartAdapterGet(void);" in adapter_header
    assert "const struct rscf_link_adapter *RSCFLinkCanAdapterGet(void);" in adapter_header
    assert "const struct rscf_link_adapter *RSCFLinkSpiAdapterGet(void);" in adapter_header

    assert "static const struct rscf_link_adapter s_usb_adapter" in usb_adapter
    assert "static const struct rscf_link_adapter s_uart_adapter" in uart_adapter
    assert "static const struct rscf_link_adapter s_can_adapter" in can_adapter
    assert "static const struct rscf_link_adapter s_spi_adapter" in spi_adapter


def test_generated_comm_and_robot_hooks_execute_in_host_harness(tmp_path):
    generated_root = REPO_ROOT / "generated" / "mcu_comm" / "MCU_ONE"
    stub_root = tmp_path / "stubs"
    (stub_root / "zephyr").mkdir(parents=True)
    (stub_root / "zephyr" / "sys").mkdir(parents=True)

    (stub_root / "zephyr" / "kernel.h").write_text(
        textwrap.dedent(
            """
            #ifndef ZEPHYR_KERNEL_H_
            #define ZEPHYR_KERNEL_H_
            #include <stdint.h>
            #define ARG_UNUSED(x) (void)(x)
            #endif
            """
        ),
        encoding="utf-8",
    )
    (stub_root / "zephyr" / "sys" / "util.h").write_text(
        textwrap.dedent(
            """
            #ifndef ZEPHYR_SYS_UTIL_H_
            #define ZEPHYR_SYS_UTIL_H_
            #define ARG_UNUSED(x) (void)(x)
            #endif
            """
        ),
        encoding="utf-8",
    )
    (stub_root / "bsp_dwt.h").write_text(
        textwrap.dedent(
            """
            #ifndef BSP_DWT_H_
            #define BSP_DWT_H_
            static inline uint32_t DWT_GetTimeline_ms(void) { return 0U; }
            #endif
            """
        ),
        encoding="utf-8",
    )
    (stub_root / "bsp_log.h").write_text(
        textwrap.dedent(
            """
            #ifndef BSP_LOG_H_
            #define BSP_LOG_H_
            #define LOGINFO(...)
            #define LOGWARNING(...)
            #define LOGERROR(...)
            #endif
            """
        ),
        encoding="utf-8",
    )

    harness_c = tmp_path / "task4_harness.c"
    harness_c.write_text(
        textwrap.dedent(
            """
            #include <stdint.h>
            #include <stdio.h>
            #include <string.h>

            #include "comm_manager.h"
            #include "robot_interface.h"
            #include "transport_can.h"
            #include "transport_uart.h"
            #include "transport_usb.h"

            static Transport_interface_t *s_transport_ready_seen;
            static uint16_t s_last_endpoint_id;
            static uint8_t s_last_pid;
            static uint8_t s_last_cmd_len;
            static uint8_t s_rx_once = 1U;

            Transport_interface_t *Comm_ResolveTransportOrDefault(Transport_interface_t *transport)
            {
                (void)transport;
                return &g_can_transport;
            }

            void RobotInterfaceOnTransportReady(Transport_interface_t *transport)
            {
                s_transport_ready_seen = transport;
            }

            void RobotInterfaceOnCompatSystemCmd(const uint8_t *control_plane_bytes, uint16_t len)
            {
                (void)control_plane_bytes;
                s_last_cmd_len = (uint8_t)len;
            }

            void Comm_OnCompatEndpointResolved(uint8_t pid, uint16_t endpoint_id)
            {
                s_last_pid = pid;
                s_last_endpoint_id = endpoint_id;
            }

            int8_t Protocol_Unpack(uint8_t byte, ProtocolFrame_t *frame)
            {
                (void)byte;
                frame->pid = PID_SYSTEM_CMD;
                return 3;
            }

            void ProtocolFrame_Init(ProtocolFrame_t *frame)
            {
                memset(frame, 0, sizeof(*frame));
            }

            uint16_t Protocol_Pack(uint8_t mid, uint8_t pid, const uint8_t *data, uint8_t len, uint8_t *out_buf)
            {
                (void)mid; (void)pid; (void)data; (void)len; (void)out_buf;
                return 0U;
            }

            uint16_t Protocol_Pack_Begin(uint8_t mid, uint8_t *out_buf)
            {
                (void)mid; (void)out_buf;
                return 0U;
            }

            uint16_t Protocol_Pack_Add(uint8_t pid, const uint8_t *data, uint8_t len, uint8_t *out_buf, uint16_t current_idx)
            {
                (void)pid; (void)data; (void)len; (void)out_buf;
                return current_idx;
            }

            uint16_t Protocol_Pack_End(uint8_t *out_buf, uint16_t current_idx)
            {
                (void)out_buf;
                return current_idx;
            }

            int stub_transport_init(void) { return 0; }
            int stub_transport_send(uint8_t *data, uint16_t len) { (void)data; (void)len; return 0; }
            int stub_transport_receive(uint8_t *buffer, uint16_t len)
            {
                if ((s_rx_once != 0U) && (len > 0U)) {
                    buffer[0] = 0x55U;
                    s_rx_once = 0U;
                    return 1;
                }
                return 0;
            }
            bool stub_transport_rx_overflowed(void) { return false; }
            void stub_transport_clear_rx_overflow(void) {}
            bool stub_transport_is_connected(void) { return true; }

            Transport_interface_t g_usb_transport = {
                .init = stub_transport_init,
                .send = stub_transport_send,
                .receive = stub_transport_receive,
                .rx_overflowed = stub_transport_rx_overflowed,
                .clear_rx_overflow = stub_transport_clear_rx_overflow,
                .is_connected = stub_transport_is_connected,
            };

            Transport_interface_t g_uart_transport = {
                .init = stub_transport_init,
                .send = stub_transport_send,
                .receive = stub_transport_receive,
                .rx_overflowed = stub_transport_rx_overflowed,
                .clear_rx_overflow = stub_transport_clear_rx_overflow,
                .is_connected = stub_transport_is_connected,
            };

            Transport_interface_t g_can_transport = {
                .init = stub_transport_init,
                .send = stub_transport_send,
                .receive = stub_transport_receive,
                .rx_overflowed = stub_transport_rx_overflowed,
                .clear_rx_overflow = stub_transport_clear_rx_overflow,
                .is_connected = stub_transport_is_connected,
            };

            int main(void)
            {
                if (!RobotInterfaceInit()) {
                    return 1;
                }

                if (s_transport_ready_seen != &g_can_transport) {
                    return 2;
                }

                RobotInterfaceProcessRx();

                if (s_last_pid != PID_SYSTEM_CMD) {
                    return 3;
                }

                if (s_last_endpoint_id != (uint16_t)(0x7000U | PID_SYSTEM_CMD)) {
                    return 4;
                }

                if (s_last_cmd_len != SYSTEM_CMD_NUM_BYTES) {
                    return 5;
                }

                return 0;
            }
            """
        ),
        encoding="utf-8",
    )

    binary = tmp_path / "task4_harness"
    compile_cmd = [
        "gcc",
        "-std=c11",
        f"-I{stub_root}",
        f"-I{generated_root / 'app'}",
        f"-I{generated_root / 'core'}",
        f"-I{generated_root / 'transports'}",
        str(harness_c),
        str(generated_root / "app" / "data_def.c"),
        str(generated_root / "core" / "comm_manager.c"),
        str(generated_root / "app" / "robot_interface.c"),
        "-o",
        str(binary),
    ]

    subprocess.run(compile_cmd, check=True, cwd=REPO_ROOT)
    subprocess.run([str(binary)], check=True, cwd=REPO_ROOT)
