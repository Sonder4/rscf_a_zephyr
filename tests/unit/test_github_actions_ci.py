from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_ci_workflow_exists() -> None:
    workflow = REPO_ROOT / ".github" / "workflows" / "ci.yml"
    assert workflow.is_file()


def test_ci_workflow_covers_repo_contracts() -> None:
    workflow = (REPO_ROOT / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")

    assert "actions/checkout@v4" in workflow
    assert "actions/setup-python@v5" in workflow
    assert "actions/upload-artifact@v4" in workflow
    assert "python -m pytest tests/unit -q" in workflow
    assert "west build -b rscf_a_f427iih6 ../app -d build_microros" in workflow
    assert "cd ros2_ws" in workflow
    assert "colcon build" in workflow
    assert "ubuntu-22.04" in workflow
    assert "ros:humble-ros-base-jammy" in workflow
