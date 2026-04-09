# Getting Started

## 1. Prerequisites

- Linux shell environment
- `python3`
- `cmake`
- network access for `west update`
- installed `Zephyr SDK`, exported with `ZEPHYR_SDK_INSTALL_DIR`

## 2. Bootstrap

```bash
cd /home/xuan/RC2026/STM32/rscf_a_zephyr
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
```

The bootstrap flow performs these actions:

1. create `.venv/`
2. install `west`
3. initialize `.workspace/` as a repo-local west workspace from the official
   `zephyr v4.3.0` manifest
4. fetch `zephyr`, `cmsis`, and `hal_stm32`
5. export the Zephyr CMake package
6. run `scripts/bootstrap/check_env.py`

## 3. Build

```bash
west build -b rscf_a_f427iih6 ../app
```

## 4. Flash

Current baseline runner configuration uses J-Link:

```bash
west flash
```

## 5. Baseline Validation

The first milestone validation is:

- successful `west build`
- UART console boot log
- status LED heartbeat path available
- board definition resolves all mandatory Zephyr metadata
