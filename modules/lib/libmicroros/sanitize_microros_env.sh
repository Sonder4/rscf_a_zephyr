#!/usr/bin/env bash

should_drop_entry() {
    local entry="$1"
    local prefix

    case "$entry" in
        /opt/ros/*)
            return 0
            ;;
    esac

    if [ -n "${VIRTUAL_ENV:-}" ]; then
        case "$entry" in
            "$VIRTUAL_ENV"|"$VIRTUAL_ENV"/*)
                return 0
                ;;
        esac
    fi

    IFS=':' read -r -a extra_prefixes <<< "${MICROROS_STRIP_PREFIXES:-}"
    for prefix in "${extra_prefixes[@]}"; do
        if [ -z "$prefix" ]; then
            continue
        fi

        case "$entry" in
            "$prefix"|"$prefix"/*)
                return 0
                ;;
        esac
    done

    return 1
}

clean_var() {
    local var_name="$1"
    local var_value filtered_value entry

    var_value="$(printenv "$var_name" 2>/dev/null || true)"
    if [ -z "$var_value" ]; then
        return
    fi

    filtered_value=""
    while IFS= read -r entry; do
        if [ -z "$entry" ]; then
            continue
        fi

        if should_drop_entry "$entry"; then
            continue
        fi

        if [ -z "$filtered_value" ]; then
            filtered_value="$entry"
        else
            filtered_value="${filtered_value}:$entry"
        fi
    done < <(printf '%s\n' "$var_value" | tr ':' '\n')

    if [ -n "$filtered_value" ]; then
        export "$var_name=$filtered_value"
    else
        unset "$var_name"
    fi
}

clean_var PATH
clean_var AMENT_PREFIX_PATH
clean_var CMAKE_PREFIX_PATH
clean_var COLCON_PREFIX_PATH
clean_var PYTHONPATH
clean_var LD_LIBRARY_PATH
