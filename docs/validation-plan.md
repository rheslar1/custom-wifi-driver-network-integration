# Validation Plan

## Current Host Checks

- CMake configure completes.
- C++17 Wi-Fi integration core builds.
- Executable accepts the nominal build/DTS/network/socket evidence scenario.
- CLI expected-failure scenarios exercise bad cross compiler, bad DTS compatible string, missing firmware, weak RSSI, and socket echo failure.
- CTest verifies each rule gate and report/telemetry evidence.
- GitHub Actions runs configure, build, executable smoke run, failure-mode checks, and CTest.

## Host Commands

```bash
cmake -S . -B build
cmake --build build
./build/custom_wifi_driver_network_integration --nominal
./build/custom_wifi_driver_network_integration --bad-cross
./build/custom_wifi_driver_network_integration --bad-dts
./build/custom_wifi_driver_network_integration --missing-firmware
./build/custom_wifi_driver_network_integration --weak-link
./build/custom_wifi_driver_network_integration --socket-fail
ctest --test-dir build --output-on-failure
```

## Hardware Evidence To Add

- Kernel module build log showing `rheslar_wifi.ko` generated with the target cross compiler.
- `modinfo` and `nm` output showing expected symbols.
- Device-tree overlay source and compiled `.dtbo` evidence.
- `dmesg` output showing firmware load and `cfg80211` PHY registration.
- `ip link`, `ip addr`, and `iw dev wlan0 link` output.
- Host PC `iperf3` or socket echo transcript.
- Packet counter delta before and after the traffic test.

## Acceptance Criteria

- Module build evidence is present and expected symbols are exported.
- DTS compatible string is `rheslar,wifi-sdio-lab`.
- Firmware path is non-empty and firmware load succeeds.
- `wlan0` is up and associated to the configured SSID.
- RSSI is at or above the target for a full pass; weak RSSI is a warning-only condition.
- Socket echo bytes sent and received match.

## Project-Specific Evidence Target

Kernel module workflow, network interface bring-up, device-tree awareness, and user-space C networking validation.
