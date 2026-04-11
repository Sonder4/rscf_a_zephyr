from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_jlink_vcom_overlay_routes_comm_uart_to_usart1():
    overlay = REPO_ROOT / "app" / "boards" / "rscf_a_f427iih6_jlink_vcom.overlay"

    assert overlay.exists()

    overlay_text = overlay.read_text(encoding="utf-8")
    assert "&rscf_board" in overlay_text
    assert "comm-uart = <&usart1>;" in overlay_text
