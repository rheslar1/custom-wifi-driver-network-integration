# Driver and Network Integration Runbook

## Target Assumptions

- ARM Linux target board with an SDIO Wi-Fi module.
- Kernel tree available on the build host.
- Cross compiler prefix: `arm-linux-gnueabihf-`.
- Driver module name: `rheslar_wifi`.
- Interface name: `wlan0`.
- Host PC endpoint: `192.168.50.10:5001`.

## Build Flow

```bash
make -C /opt/linux-6.6.32 \
  M=/work/drivers/rheslar_wifi \
  ARCH=arm \
  CROSS_COMPILE=arm-linux-gnueabihf- \
  WERROR=1 \
  modules
```

Required exported symbols:

- `rheslar_wifi_probe`
- `rheslar_wifi_remove`
- `rheslar_wifi_xmit`

## Device Tree Overlay

The host model expects:

```dts
compatible = "rheslar,wifi-sdio-lab";
max-frequency = <50000000>;
interrupt-gpios = <&gpio1 18 GPIO_ACTIVE_HIGH>;
reset-gpios = <&gpio1 19 GPIO_ACTIVE_LOW>;
firmware-name = "rheslar_wifi/fw.bin";
local-mac-address = [02 52 48 45 53 01];
```

Compile with:

```bash
dtc -@ -I dts -O dtb -o rheslar-wifi0.dtbo rheslar-wifi0.dts
```

## Bring-Up Flow

```bash
modprobe cfg80211
insmod rheslar_wifi.ko
dmesg | grep -E 'rheslar_wifi|cfg80211|firmware'
ip link set wlan0 up
wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wlan0.conf
ip addr add 192.168.50.44/24 dev wlan0
iw dev wlan0 link
ip -s link show wlan0
iperf3 -c 192.168.50.10 -p 5001
```

## CI Model Mapping

The C++17 host model maps those target checks into deterministic structures:

- `KernelBuildConfig` models the `make -C` command and exported symbols.
- `DeviceTreeOverlay` models the `.dts` content.
- `NetworkProfile` models `wlan0`, SSID, address, and host PC endpoint.
- `DriverObservation` models the evidence collected from `dmesg`, `ip`, `iw`, and socket traffic.
- `IIntegrationRule` strategies validate each boundary independently.
