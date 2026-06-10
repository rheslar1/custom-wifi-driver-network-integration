# Custom Wi-Fi Driver Compilation & Network Integration

Kernel-module and network-stack project that cross-compiles Wi-Fi driver support, updates device tree integration, and validates socket traffic with a host PC.

## Portfolio Purpose

This repository implements a host-testable model of a custom Wi-Fi driver integration workflow. It does not require a kernel tree during CI, but it models the evidence expected from a real bring-up: cross-compiling a `.ko`, validating exported symbols, applying a device-tree overlay, loading firmware, registering a `cfg80211` PHY, bringing `wlan0` up, associating to an AP, and proving socket traffic against a host PC.

The implementation is C++17-first and uses design patterns and SOLID boundaries so the simulated CI model can be replaced by target adapters later.

## Stack

- C++17
- C++ Design Patterns
- SOLID
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
ctest --test-dir build --output-on-failure
```

Failure-mode scenarios:

```bash
./build/custom_wifi_driver_network_integration --bad-cross
./build/custom_wifi_driver_network_integration --bad-dts
./build/custom_wifi_driver_network_integration --missing-firmware
./build/custom_wifi_driver_network_integration --weak-link
./build/custom_wifi_driver_network_integration --socket-fail
```

## Implementation Slices

- C++17 model for kernel module build configuration, device-tree overlay, network profile, and observed bring-up state.
- Strategy validation rules for cross-compile config, device-tree binding, module/firmware load, Wi-Fi association, and socket traffic.
- Composite validator and runner facade that produce deterministic pass/fail integration reports.
- Linux command-plan generator covering `make -C`, `dtc`, firmware/module install, `modprobe`, `insmod`, `ip`, `wpa_supplicant`, and `iperf3`.
- JSON-style telemetry payload and text report suitable for CI logs and portfolio evidence.
- CTest coverage for nominal bring-up, bad cross compiler, bad DTS compatible string, missing firmware, weak RSSI warning, socket failure, and report evidence.

## C++17 Design Patterns and SOLID

| Pattern | Implementation |
| --- | --- |
| Strategy | `IIntegrationRule` implementations validate each integration gate independently. |
| Composite | `CompositeRuleSet` evaluates the ordered rule chain. |
| Adapter | `ICommandPlanBuilder` and `ITelemetryEncoder` isolate command generation and output encoding. |
| Facade | `WifiIntegrationRunner` is the single orchestration entry point. |
| DTO / Value Object | Build, overlay, network, observation, issue, and report structs keep data explicit. |

SOLID mapping:

- Single Responsibility: build config, DTS, network profile, observations, validation rules, command plan, and report writing are separate.
- Open/Closed: new rules can be added without editing the runner.
- Liskov Substitution: production command-plan or telemetry encoders can replace the host implementations.
- Interface Segregation: rule, command-plan, and telemetry interfaces are narrow.
- Dependency Inversion: orchestration depends on interfaces rather than concrete Linux commands.

## Evidence Target

Kernel module workflow, network interface bring-up, device-tree awareness, and user-space C networking validation.

## Remote

Intended public repository: https://github.com/rheslar1/custom-wifi-driver-network-integration

<!-- cpp17-solid-implementation:start -->
## C++17, Design Patterns, and SOLID Implementation

This repository includes a host-buildable C++17 implementation, not only documentation. The implementation applies:

- Strategy pattern for validation rules.
- Adapter interfaces for input samples and telemetry/reporting.
- Composite validation for combining safety and readiness checks.
- Facade orchestration through the project runtime class.
- SOLID boundaries between profile data, input acquisition, validation, telemetry encoding, and tests.
<!-- cpp17-solid-implementation:end -->

<!-- deep-architecture-links:start -->
## Deep Architecture and UML

- [Deep architecture](docs/deep-architecture.md)
- [Full UML Draw.io source](docs/diagrams/full-system-uml.drawio)
- [Full UML PNG export](docs/diagrams/full-system-uml.png)
<!-- deep-architecture-links:end -->
