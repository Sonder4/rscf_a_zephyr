from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_board_maps_buzzer_to_tim12_and_declares_power_switch_bank() -> None:
    board_dts = (
        REPO_ROOT / "boards" / "arm" / "rscf_a_f427iih6" / "rscf_a_f427iih6.dts"
    ).read_text(encoding="utf-8")
    modules_kconfig = (REPO_ROOT / "modules" / "rscf" / "Kconfig").read_text(encoding="utf-8")
    power_driver = (
        REPO_ROOT / "drivers" / "rscf" / "rscf_power_switch.c"
    ).read_text(encoding="utf-8")

    assert 'pwms = <&pwm12 0 PWM_MSEC(2) PWM_POLARITY_NORMAL>;' in board_dts
    assert "&timers12 {" in board_dts
    assert "pwm12: pwm {" in board_dts
    assert "pinctrl-0 = <&tim12_ch1_ph6>;" in board_dts

    assert 'compatible = "ncurc,rscf-power-switch-bank";' in board_dts
    assert "power-gpios = <&gpioh 2 GPIO_ACTIVE_HIGH>," in board_dts
    assert "<&gpioh 3 GPIO_ACTIVE_HIGH>," in board_dts
    assert "<&gpioh 4 GPIO_ACTIVE_HIGH>," in board_dts
    assert "<&gpioh 5 GPIO_ACTIVE_HIGH>;" in board_dts

    assert "config RSCF_POWER_SWITCH" in modules_kconfig
    assert "RSCFPowerSwitchSetMask" in power_driver
    assert "GPIO_DT_SPEC_GET_BY_IDX" in power_driver
