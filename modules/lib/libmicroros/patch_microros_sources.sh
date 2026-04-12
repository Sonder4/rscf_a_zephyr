#!/usr/bin/env bash

set -euo pipefail

component_path="${1:?component path is required}"
src_root="${component_path}/micro_ros_src/src"
patch_root="${component_path}/patches"
template_root="${component_path}/templates"

ensure_local_copy() {
    local path="$1"
    local target

    if [ ! -L "$path" ]; then
        return
    fi

    target="$(readlink -f "$path")"
    rm "$path"
    cp -a "$target" "$path"
}

apply_patch_if_needed() {
    local package_dir="$1"
    local patch_file="$2"

    if \
        grep -Fq "#include <strings.h>" "${package_dir}/src/strcasecmp.c" && \
        grep -Fq "IS_STREAM_A_TTY(stream) (false)" "${package_dir}/src/logging.c"; then
        return
    fi

    if patch -d "$package_dir" -N -p1 < "$patch_file"; then
        return
    fi

    if \
        grep -Fq "#include <strings.h>" "${package_dir}/src/strcasecmp.c" && \
        grep -Fq "IS_STREAM_A_TTY(stream) (false)" "${package_dir}/src/logging.c"; then
        return
    fi

    return 1
}

apply_tree_patch_if_needed() {
    local tree_root="$1"
    local patch_file="$2"

    if \
        grep -Fq "abort();" "${tree_root}/rosidl/rosidl_runtime_c/src/string_functions.c" && \
        grep -Fq "abort();" "${tree_root}/rosidl/rosidl_runtime_c/src/u16string_functions.c" && \
        grep -Fq "abort();" "${tree_root}/rcl/rcl/src/rcl/rmw_implementation_identifier_check.c"; then
        return
    fi

    patch -d "$tree_root" -N -p1 < "$patch_file" >/dev/null 2>&1 || true
    find "$tree_root" -name '*.rej' -delete 2>/dev/null || true
}

fix_embedded_exit_sources() {
    local tree_root="$1"
    local string_file="${tree_root}/rosidl/rosidl_runtime_c/src/string_functions.c"
    local u16_file="${tree_root}/rosidl/rosidl_runtime_c/src/u16string_functions.c"
    local rmw_check_file="${tree_root}/rcl/rcl/src/rcl/rmw_implementation_identifier_check.c"

    sed -i \
        -e '/#include <stdio.h>/d' \
        -e '/fprintf(/d' \
        -e '/stderr, "Unexpected /d' \
        -e '/Exiting/d' \
        -e 's/exit(-1);/abort();/g' \
        "$string_file" \
        "$u16_file"

    sed -i 's/exit(ret);/abort();/' "$rmw_check_file"

    if \
        grep -Fq "exit(" "$string_file" || \
        grep -Fq "fprintf(" "$string_file" || \
        grep -Fq "exit(" "$u16_file" || \
        grep -Fq "fprintf(" "$u16_file" || \
        grep -Fq "exit(ret);" "$rmw_check_file"; then
        return 1
    fi
}

sync_time_unix_template() {
    local package_dir="$1"
    local template_file="$2"
    local target_file="${package_dir}/src/time_unix.c"

    if cmp -s "$template_file" "$target_file"; then
        return
    fi

    cp "$template_file" "$target_file"
}

disable_optional_packages() {
    local src_dir="$1"

    touch "${src_dir}/rosidl_dynamic_typesupport/COLCON_IGNORE" 2>/dev/null || true
    touch "${src_dir}/rosidl_core/COLCON_IGNORE" 2>/dev/null || true
}

ensure_local_copy "${src_root}/rcutils"
ensure_local_copy "${src_root}/rosidl"
ensure_local_copy "${src_root}/rcl"
disable_optional_packages "${src_root}"
apply_patch_if_needed \
    "${src_root}/rcutils" \
    "${patch_root}/rcutils-zephyr4.patch"
apply_tree_patch_if_needed \
    "${src_root}" \
    "${patch_root}/embedded-no-exit.patch"
fix_embedded_exit_sources "${src_root}"
sync_time_unix_template \
    "${src_root}/rcutils" \
    "${template_root}/rcutils_time_unix_zephyr.c"
