from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_ros_backend_choice_defaults_to_bridge() -> None:
    kconfig = (REPO_ROOT / "modules" / "rscf" / "Kconfig").read_text(encoding="utf-8")

    assert "config RSCF_LINK_VNEXT" in kconfig
    assert "config RSCF_LINK_COMPAT_SYSTEM_CMD" in kconfig
    assert "choice RSCF_ROS_BACKEND" in kconfig
    assert "default RSCF_ROS_BACKEND_BRIDGE" in kconfig
    assert "config RSCF_ROS_BACKEND_BRIDGE" in kconfig
    assert "config RSCF_ROS_BACKEND_MICROROS" in kconfig


def test_prj_conf_selects_bridge_backend() -> None:
    prj_conf = (REPO_ROOT / "app" / "prj.conf").read_text(encoding="utf-8")

    assert "CONFIG_RSCF_LINK_VNEXT=y" in prj_conf
    assert "CONFIG_RSCF_ROS_BACKEND_BRIDGE=y" in prj_conf
    assert "CONFIG_RSCF_ROS_BACKEND_MICROROS=n" in prj_conf
    assert "CONFIG_RSCF_COMM_SERVICE=y" in prj_conf
    assert "CONFIG_RSCF_ROS_BACKEND_MICROROS=y" not in prj_conf
    assert "CONFIG_MICROROS=" not in prj_conf


def test_devicetree_links_have_host_and_peer_bindings() -> None:
    board_dts = (REPO_ROOT / "boards" / "arm" / "rscf_a_f427iih6" / "rscf_a_f427iih6.dts").read_text(
        encoding="utf-8"
    )
    app_overlay = (REPO_ROOT / "app" / "app.overlay").read_text(encoding="utf-8")
    board_overlay = (
        REPO_ROOT / "app" / "boards" / "rscf_a_f427iih6.overlay"
    ).read_text(encoding="utf-8")

    assert "rscf,peer-link" in board_dts
    assert "rscf-peer-uart" in board_dts
    assert "cdc_acm_uart0: cdc_acm_uart0" in app_overlay
    assert "rscf,host-link" in board_overlay
    assert "rscf-host-usb" in board_overlay
    assert "cdc_acm_uart0: cdc_acm_uart0" not in board_overlay
    assert app_overlay.count("cdc_acm_uart0: cdc_acm_uart0") == 1
    assert board_overlay.count("rscf,host-link") == 1
    assert board_overlay.count("rscf-host-usb") == 1
