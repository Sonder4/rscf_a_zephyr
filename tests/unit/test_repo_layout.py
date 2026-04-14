from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_manifest_and_bootstrap_files_exist() -> None:
    assert (REPO_ROOT / "west.yml").is_file()
    assert (REPO_ROOT / "scripts" / "bootstrap" / "bootstrap.sh").is_file()
    assert (REPO_ROOT / "scripts" / "bootstrap" / "check_env.py").is_file()
    assert (REPO_ROOT / "docs" / "getting-started.md").is_file()


def test_board_definition_files_exist() -> None:
    board_dir = REPO_ROOT / "boards" / "arm" / "rscf_a_f427iih6"

    assert (board_dir / "board.yml").is_file()
    assert (board_dir / "Kconfig.rscf_a_f427iih6").is_file()
    assert (board_dir / "Kconfig.defconfig").is_file()
    assert (board_dir / "board.cmake").is_file()
    assert (board_dir / "rscf_a_f427iih6_defconfig").is_file()
    assert (board_dir / "rscf_a_f427iih6.dts").is_file()
    assert (board_dir / "rscf_a_f427iih6-pinctrl.dtsi").is_file()


def test_manifest_pins_zephyr_43() -> None:
    west_yml = (REPO_ROOT / "west.yml").read_text(encoding="utf-8")

    assert "name: zephyr" in west_yml
    assert "revision: v4.3.0" in west_yml
    assert "path: zephyr" in west_yml


def test_app_has_minimal_entrypoint() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")
    app_kconfig = (REPO_ROOT / "app" / "Kconfig").read_text(encoding="utf-8")
    prj_conf = (REPO_ROOT / "app" / "prj.conf").read_text(encoding="utf-8")
    main_c = (REPO_ROOT / "app" / "src" / "main.c").read_text(encoding="utf-8")

    assert "find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})" in app_cmake
    assert "project(rscf_a_zephyr)" in app_cmake
    assert 'rsource "../modules/rscf/Kconfig"' in app_kconfig
    assert "CONFIG_GPIO=y" in prj_conf
    assert "CONFIG_SERIAL=y" in prj_conf
    assert "int main(void)" in main_c
    assert "RSCF A Zephyr bring-up" in main_c


def test_bootstrap_uses_repo_local_workspace() -> None:
    bootstrap = (REPO_ROOT / "scripts" / "bootstrap" / "bootstrap.sh").read_text(encoding="utf-8")
    getting_started = (REPO_ROOT / "docs" / "getting-started.md").read_text(encoding="utf-8")

    assert ".workspace" in bootstrap
    assert ".workspace" in getting_started
    assert "--mf west.yml" in bootstrap


def test_segger_module_is_vendored_in_repo_root() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")
    west_yml = (REPO_ROOT / "west.yml").read_text(encoding="utf-8")

    assert (REPO_ROOT / "modules" / "debug" / "segger" / "zephyr" / "module.yml").is_file()
    assert "../modules/debug/segger" in app_cmake
    assert ".workspace/modules/debug/segger" not in app_cmake
    assert "name: segger" not in west_yml


def test_link_vnext_runtime_skeleton_files_exist() -> None:
    assert (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_link_service.c").is_file()
    assert (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_link_service.h").is_file()
    assert (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_service_router.c").is_file()
    assert (REPO_ROOT / "modules" / "rscf" / "services" / "rscf_action_service.c").is_file()
