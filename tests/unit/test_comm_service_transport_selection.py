from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_comm_service_respects_generated_default_transport():
    service_text = (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_comm_service.c").read_text(
        encoding="utf-8"
    )

    assert "MCU_COMM_DEFAULT_TRANSPORT" in service_text
    assert "MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_USB" in service_text
    assert "s_selected_transport = &g_uart_transport;" in service_text
