# RSCF A Zephyr

`RSCF A Zephyr` is the standalone Zephyr RTOS migration repository for the
STM32F427IIH6-based RSCF A controller.

## Scope

- Zephyr baseline: `v4.3.0`
- Workspace tool: `west`
- Standard toolchain: `Zephyr SDK`
- Target board: `rscf_a_f427iih6`
- Migration style: standalone repository, not an in-place FreeRTOS rewrite

## Repository Layout

- `app/`: Zephyr application entrypoint and Kconfig integration
- `boards/arm/rscf_a_f427iih6/`: custom board definition
- `drivers/rscf/`: RSCF board-facing driver adapters
- `modules/rscf/`: RSCF services and device modules
- `profiles/`: static application profile options
- `scripts/bootstrap/`: workspace and toolchain bootstrap helpers
- `dts/bindings/rscf/`: custom devicetree bindings
- `docs/`: migration and bring-up documentation

## Quick Start

```bash
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
west build -b rscf_a_f427iih6 ../app
```

## Current Status

This repository currently provides:

- the workspace manifest and bootstrap flow
- the custom board skeleton for `STM32F427IIH6`
- a minimal Zephyr bring-up application
- initial RSCF service and profile abstractions

Hardware-parity migration of the full legacy BSP, Modules, and APP stacks will
be layered on top of this baseline.

## Bootstrap Model

`./scripts/bootstrap/bootstrap.sh` creates a repo-local Zephyr workspace under
`.workspace/` using the official `zephyr v4.3.0` manifest. This avoids turning
the repository parent directory into a west workspace while still allowing this
repository to supply the out-of-tree board, DTS bindings, drivers, and app
sources.
