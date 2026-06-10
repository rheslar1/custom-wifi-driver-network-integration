# Custom Wi-Fi Driver Compilation & Network Integration Architecture

## Goal

Kernel module workflow, network interface bring-up, device-tree awareness, and user-space C networking validation.

## System Flow

1. Define the kernel module build profile: module name, target kernel, target architecture, cross compiler, kernel source path, module source path, and expected exported symbols.
2. Validate device-tree overlay data: compatible string, bus type, SDIO clock, IRQ/reset GPIOs, firmware path, regulatory domain, power sequencing, and interface name.
3. Validate driver load evidence: `.ko` was produced, module inserted, firmware loaded, `cfg80211` PHY registered, and expected symbols exported.
4. Validate network state: interface up, CIDR address configured, station associated with the requested SSID, RSSI above target, and MTU inside Wi-Fi bounds.
5. Validate host PC socket traffic: endpoint configured, packet counters incremented, and echoed byte counts match.
6. Emit a Linux command plan and telemetry payload for review.

## Runtime Components

| Component | Responsibility |
| --- | --- |
| `KernelBuildConfig` | Cross-compile and module-build contract. |
| `DeviceTreeOverlay` | Board integration contract for SDIO/SPI/PCIe/USB Wi-Fi modules. |
| `NetworkProfile` | Interface, SSID, auth, IP, and host-PC traffic target. |
| `DriverObservation` | Captured evidence from build logs, `dmesg`, `ip`, `iw`, and socket tests. |
| `IIntegrationRule` | Strategy interface for independent validation rules. |
| `CompositeRuleSet` | Composite pattern for ordered rule execution. |
| `LinuxCommandPlanBuilder` | Adapter for producing target commands without executing them in CI. |
| `WifiIntegrationRunner` | Facade that combines inputs, rules, command plan, and telemetry. |

## C++17 Design Shape

- Strategy pattern: each validation rule implements `IIntegrationRule`.
- Composite pattern: `CompositeRuleSet` owns and evaluates rules in order.
- Adapter pattern: command-plan and telemetry encoders are replaceable interfaces.
- Facade pattern: `WifiIntegrationRunner::run()` is the project review entry point.
- Value-object style data contracts keep build, DTS, network, and observations explicit.

## SOLID Notes

- Single Responsibility: build validation, DTS validation, module validation, association validation, socket validation, command planning, and reporting are separate types.
- Open/Closed: new checks can be added as new `IIntegrationRule` implementations.
- Liskov Substitution: host telemetry/command plan adapters can be replaced by hardware-backed implementations.
- Interface Segregation: rules, command builders, and telemetry encoders have focused APIs.
- Dependency Inversion: the runner depends on abstractions and injected collaborators.

## Evidence Sources

The host model maps to these target-side evidence sources:

- `make -C <kernel> M=<module> ARCH=<arch> CROSS_COMPILE=<prefix> modules`
- `modinfo`, `nm`, and `dmesg` for module and symbol evidence
- `dtc -@` and `/sys/firmware/devicetree` for overlay evidence
- `modprobe cfg80211`, `insmod`, and firmware loader logs
- `ip link`, `ip addr`, `iw dev wlan0 link`, and packet counters
- `iperf3`, TCP echo, or UDP echo test with a host PC

## Validation Plan

- Build the C++17 host model with CMake.
- Run the nominal executable and confirm the integration report passes.
- Run failure-mode CLI scenarios for bad cross compiler, bad DTS, missing firmware, weak RSSI, and socket failure.
- Run CTest to validate all rule gates and report evidence.
- Add hardware logs from the target board after real driver bring-up.

## Expansion Path

- Replace `DriverObservation` fixtures with parsers for real build logs, `dmesg`, `iw`, `ip -s link`, and socket-test output.
- Add board-specific DTS overlays for the selected SDIO/SPI/PCIe Wi-Fi module.
- Add a tiny C socket echo tool for the target and a host PC script for repeatable traffic evidence.
- Add kernel module source once the exact chipset and kernel version are selected.

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
