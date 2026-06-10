# Custom Wi-Fi Driver Compilation & Network Integration

Kernel-module and network-stack project that cross-compiles Wi-Fi driver support, updates device tree integration, and validates socket traffic with a host PC.

## Portfolio Purpose

This repository is an Embedded Systems project scaffold for the Rheslar portfolio. It is designed to become a hardware-backed project with build output, validation logs, and reviewable implementation evidence.

## Stack

- Linux kernel
- Wi-Fi driver
- Kernel module
- Device tree
- Cross-compilation
- Sockets

## Quick Start

```bash
cmake -S . -B build
cmake --build build
./build/custom_wifi_driver_network_integration
python -m unittest discover -s tests
```

## Implementation Slices

- Native starter executable that exposes the project identity, stack, and validation target.
- Architecture document with control boundaries, data flow, safety assumptions, and evidence plan.
- Unit smoke test that keeps source, docs, and CI files present as the repo grows.
- GitHub Actions workflow for configure, build, executable smoke run, and repository validation.

## Evidence Target

Kernel module workflow, network interface bring-up, device-tree awareness, and user-space C networking validation.

## Remote

Intended public repository: https://github.com/rheslar1/custom-wifi-driver-network-integration
