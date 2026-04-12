from pathlib import Path
import subprocess
import textwrap


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
    assert "RSCF_LINK_SERVICE_POLL_PERIOD_MS" in link_service_h
    assert "RSCFLinkServiceInit" in link_service_c
    assert "RSCFLinkServiceProcess" in link_service_c
    assert "RSCFLinkServiceGetRuntime" in link_service_c
    assert "SYS_INIT(rscf_link_service_auto_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);" in link_service_c
    assert "k_msgq_put(&s_runtime.event_queue, &event, K_NO_WAIT) == 0" in link_service_c
    assert "s_runtime.dropped_events++;" in link_service_c
    assert "k_work_reschedule(&s_runtime.poll_work, K_MSEC(RSCF_LINK_SERVICE_POLL_PERIOD_MS))" in link_service_c
    assert "RSCFServiceRouterInit" in service_router_c
    assert "RSCFServiceRouterDispatch" in service_router_c
    assert "RSCFActionServiceInit" in action_service_c
    assert "RSCFActionServiceProcess" in action_service_c
    assert "runtime->handled_events >= runtime->last_event_sequence" in action_service_c


def test_link_runtime_skeleton_exercises_poll_router_action_path(tmp_path):
    stub_root = tmp_path / "stubs"
    (stub_root / "zephyr" / "logging").mkdir(parents=True)
    (stub_root / "zephyr" / "sys").mkdir(parents=True)
    (stub_root / "zephyr").mkdir(exist_ok=True)

    (stub_root / "zephyr" / "kernel.h").write_text(
        textwrap.dedent(
            """
            #ifndef ZEPHYR_KERNEL_H_
            #define ZEPHYR_KERNEL_H_
            #include <stddef.h>
            #include <stdint.h>
            #include <string.h>

            #define K_NO_WAIT 0
            #define K_MSEC(ms) (ms)
            #define APPLICATION 0
            #define CONFIG_APPLICATION_INIT_PRIORITY 0
            #define ARG_UNUSED(x) (void)(x)

            struct k_work {
                void *unused;
            };

            struct k_work_delayable {
                struct k_work work;
                void (*handler)(struct k_work *work);
            };

            struct k_msgq {
                char *buffer;
                size_t msg_size;
                uint32_t max_msgs;
                uint32_t used_msgs;
            };

            static int stub_in_work_handler;

            static inline void k_work_init_delayable(struct k_work_delayable *work,
                                                     void (*handler)(struct k_work *work))
            {
                work->handler = handler;
            }

            static inline int k_work_reschedule(struct k_work_delayable *work, int delay)
            {
                ARG_UNUSED(delay);
                if ((work->handler != NULL) && (stub_in_work_handler == 0)) {
                    stub_in_work_handler = 1;
                    work->handler(&work->work);
                    stub_in_work_handler = 0;
                }
                return 0;
            }

            static inline void k_msgq_init(struct k_msgq *queue,
                                           char *buffer,
                                           size_t msg_size,
                                           uint32_t max_msgs)
            {
                queue->buffer = buffer;
                queue->msg_size = msg_size;
                queue->max_msgs = max_msgs;
                queue->used_msgs = 0;
            }

            static inline int k_msgq_put(struct k_msgq *queue, const void *data, int timeout)
            {
                ARG_UNUSED(timeout);
                if (queue->used_msgs >= queue->max_msgs) {
                    return -1;
                }

                memcpy(queue->buffer + (queue->used_msgs * queue->msg_size), data, queue->msg_size);
                queue->used_msgs++;
                return 0;
            }

            static inline int k_msgq_get(struct k_msgq *queue, void *data, int timeout)
            {
                ARG_UNUSED(timeout);
                if (queue->used_msgs == 0U) {
                    return -1;
                }

                memcpy(data, queue->buffer, queue->msg_size);
                if (queue->used_msgs > 1U) {
                    memmove(queue->buffer,
                            queue->buffer + queue->msg_size,
                            (queue->used_msgs - 1U) * queue->msg_size);
                }
                queue->used_msgs--;
                return 0;
            }
            #endif
            """
        ),
        encoding="utf-8",
    )

    (stub_root / "zephyr" / "sys" / "ring_buffer.h").write_text(
        textwrap.dedent(
            """
            #ifndef ZEPHYR_SYS_RING_BUFFER_H_
            #define ZEPHYR_SYS_RING_BUFFER_H_
            #include <stdint.h>
            #include <string.h>

            struct ring_buf {
                uint8_t *buffer;
                uint32_t size;
                uint32_t head;
                uint32_t tail;
            };

            static inline void ring_buf_init(struct ring_buf *ring, uint32_t size, uint8_t *buffer)
            {
                ring->buffer = buffer;
                ring->size = size;
                ring->head = 0U;
                ring->tail = 0U;
            }

            #endif
            """
        ),
        encoding="utf-8",
    )

    (stub_root / "zephyr" / "logging" / "log.h").write_text(
        textwrap.dedent(
            """
            #ifndef ZEPHYR_LOGGING_LOG_H_
            #define ZEPHYR_LOGGING_LOG_H_
            #define LOG_MODULE_REGISTER(name, level)
            #define LOG_INF(...)
            #define LOG_DBG(...)
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
            #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
            #endif
            """
        ),
        encoding="utf-8",
    )

    (stub_root / "zephyr" / "init.h").write_text(
        textwrap.dedent(
            """
            #ifndef ZEPHYR_INIT_H_
            #define ZEPHYR_INIT_H_
            #define SYS_INIT(fn, level, prio)
            #endif
            """
        ),
        encoding="utf-8",
    )

    harness_c = tmp_path / "runtime_harness.c"
    harness_c.write_text(
        textwrap.dedent(
            """
            #include "rscf_link_service.h"

            int main(void)
            {
                struct rscf_link_runtime *runtime;

                if (RSCFLinkServiceInit() != 0) {
                    return 1;
                }

                runtime = RSCFLinkServiceGetRuntime();
                if ((runtime == 0) || !runtime->ready) {
                    return 2;
                }

                if (runtime->scheduled_events == 0U) {
                    return 3;
                }

                if (runtime->last_event_sequence == 0U) {
                    return 4;
                }

                if (runtime->handled_events == 0U) {
                    return 5;
                }

                RSCFLinkServiceProcess();
                if (runtime->handled_events == 0U) {
                    return 6;
                }

                return 0;
            }
            """
        ),
        encoding="utf-8",
    )

    binary = tmp_path / "runtime_harness"
    service_dir = REPO_ROOT / "modules" / "rscf" / "services"
    compile_cmd = [
        "gcc",
        "-std=c11",
        f"-I{stub_root}",
        f"-I{service_dir}",
        str(harness_c),
        str(service_dir / "rscf_link_service.c"),
        str(service_dir / "rscf_service_router.c"),
        str(service_dir / "rscf_action_service.c"),
        "-o",
        str(binary),
    ]

    subprocess.run(compile_cmd, check=True, cwd=REPO_ROOT)
    subprocess.run([str(binary)], check=True, cwd=REPO_ROOT)
