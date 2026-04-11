from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def test_repo_vendors_cmsis_dsp_sources() -> None:
    expected = [
        "third_party/cmsis_dsp/Include/arm_math.h",
        "third_party/cmsis_dsp/Include/dsp/fast_math_functions.h",
        "third_party/cmsis_dsp/Include/dsp/matrix_functions.h",
        "third_party/cmsis_dsp/PrivateInclude",
        "third_party/cmsis_dsp/Source/CommonTables/arm_common_tables.c",
        "third_party/cmsis_dsp/Source/FastMathFunctions/arm_sin_f32.c",
        "third_party/cmsis_dsp/Source/FastMathFunctions/arm_cos_f32.c",
        "third_party/cmsis_dsp/Source/FastMathFunctions/arm_atan2_f32.c",
        "third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_init_f32.c",
        "third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_add_f32.c",
        "third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_sub_f32.c",
        "third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_mult_f32.c",
        "third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_trans_f32.c",
        "third_party/cmsis_dsp/Source/MatrixFunctions/arm_mat_inverse_f32.c",
    ]

    for relpath in expected:
        assert (REPO_ROOT / relpath).exists(), relpath


def test_app_cmake_uses_cmsis_dsp_and_not_local_arm_math_compat() -> None:
    app_cmake = (REPO_ROOT / "app" / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "third_party/cmsis_dsp/Include" in app_cmake
    assert "third_party/cmsis_dsp/PrivateInclude" in app_cmake
    assert "arm_common_tables.c" in app_cmake
    assert "arm_sin_f32.c" in app_cmake
    assert "arm_cos_f32.c" in app_cmake
    assert "arm_atan2_f32.c" in app_cmake
    assert "arm_mat_init_f32.c" in app_cmake
    assert "arm_mat_add_f32.c" in app_cmake
    assert "arm_mat_sub_f32.c" in app_cmake
    assert "arm_mat_mult_f32.c" in app_cmake
    assert "arm_mat_trans_f32.c" in app_cmake
    assert "arm_mat_inverse_f32.c" in app_cmake
    assert "ARM_MATH_CM4" in app_cmake
    assert "ARM_MATH_MATRIX_CHECK" in app_cmake
    assert "arm_math_compat.c" not in app_cmake

