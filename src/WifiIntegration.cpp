#include "wifi_integration/WifiIntegration.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace wifi_integration {
namespace {

bool startsWith(std::string_view value, std::string_view prefix) {
  return value.substr(0U, prefix.size()) == prefix;
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

std::string jsonEscape(const std::string& value) {
  std::ostringstream escaped;
  for (const char item : value) {
    switch (item) {
      case '\\':
        escaped << "\\\\";
        break;
      case '"':
        escaped << "\\\"";
        break;
      case '\n':
        escaped << "\\n";
        break;
      case '\r':
        escaped << "\\r";
        break;
      case '\t':
        escaped << "\\t";
        break;
      default:
        escaped << item;
        break;
    }
  }
  return escaped.str();
}

std::string fixed2(const double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  return stream.str();
}

IntegrationIssue issue(IntegrationStage stage,
                       Severity severity,
                       std::string code,
                       std::string detail) {
  return IntegrationIssue{stage, severity, std::move(code), std::move(detail)};
}

bool validCidr(std::string_view cidr) {
  const auto slash = cidr.find('/');
  return slash != std::string_view::npos && slash > 0U && slash + 1U < cidr.size();
}

std::string quote(const std::string& value) {
  return "'" + value + "'";
}

}  // namespace

std::string toString(const BusType bus) {
  switch (bus) {
    case BusType::Sdio:
      return "sdio";
    case BusType::Spi:
      return "spi";
    case BusType::Pcie:
      return "pcie";
    case BusType::Usb:
      return "usb";
  }
  return "unknown";
}

std::string toString(const AuthMode mode) {
  switch (mode) {
    case AuthMode::Open:
      return "open";
    case AuthMode::Wpa2Psk:
      return "wpa2-psk";
    case AuthMode::Wpa3Sae:
      return "wpa3-sae";
    case AuthMode::Enterprise:
      return "enterprise";
  }
  return "unknown";
}

std::string toString(const Severity severity) {
  switch (severity) {
    case Severity::Info:
      return "info";
    case Severity::Warning:
      return "warning";
    case Severity::Critical:
      return "critical";
  }
  return "unknown";
}

std::string toString(const IntegrationStage stage) {
  switch (stage) {
    case IntegrationStage::Build:
      return "build";
    case IntegrationStage::DeviceTree:
      return "device-tree";
    case IntegrationStage::ModuleLoad:
      return "module-load";
    case IntegrationStage::FirmwareLoad:
      return "firmware-load";
    case IntegrationStage::InterfaceBringup:
      return "interface-bringup";
    case IntegrationStage::Association:
      return "association";
    case IntegrationStage::SocketTraffic:
      return "socket-traffic";
  }
  return "unknown";
}

std::optional<IntegrationIssue> BuildConfigurationRule::evaluate(
    const KernelBuildConfig& build,
    const DeviceTreeOverlay&,
    const NetworkProfile&,
    const DriverObservation& observation) const {
  if (build.moduleName.empty() || build.kernelRelease.empty()) {
    return issue(IntegrationStage::Build,
                 Severity::Critical,
                 "BUILD_IDENTITY_MISSING",
                 "module name and kernel release are required");
  }

  if (build.architecture.empty() || build.crossCompilerPrefix.empty()) {
    return issue(IntegrationStage::Build,
                 Severity::Critical,
                 "CROSS_COMPILE_MISSING",
                 "target architecture and CROSS_COMPILE prefix are required");
  }

  if (!startsWith(build.crossCompilerPrefix, "arm-linux-") &&
      !startsWith(build.crossCompilerPrefix, "aarch64-linux-")) {
    return issue(IntegrationStage::Build,
                 Severity::Critical,
                 "CROSS_COMPILE_UNSUPPORTED",
                 "expected an ARM Linux cross compiler prefix");
  }

  if (build.kernelSourcePath.empty() || build.moduleSourcePath.empty()) {
    return issue(IntegrationStage::Build,
                 Severity::Critical,
                 "KERNEL_SOURCE_MISSING",
                 "kernel source and module source paths are required");
  }

  if (!observation.moduleBuilt) {
    return issue(IntegrationStage::Build,
                 Severity::Critical,
                 "MODULE_NOT_BUILT",
                 build.moduleName + ".ko was not produced");
  }

  for (const auto& symbol : build.expectedSymbols) {
    if (!contains(observation.exportedSymbols, symbol)) {
      return issue(IntegrationStage::Build,
                   Severity::Critical,
                   "SYMBOL_MISSING",
                   "expected exported symbol " + symbol + " was not found");
    }
  }

  return std::nullopt;
}

std::string BuildConfigurationRule::name() const {
  return "BuildConfigurationRule";
}

std::optional<IntegrationIssue> DeviceTreeRule::evaluate(
    const KernelBuildConfig&,
    const DeviceTreeOverlay& overlay,
    const NetworkProfile& network,
    const DriverObservation&) const {
  if (overlay.compatible != "rheslar,wifi-sdio-lab") {
    return issue(IntegrationStage::DeviceTree,
                 Severity::Critical,
                 "COMPATIBLE_MISMATCH",
                 "device-tree compatible must be rheslar,wifi-sdio-lab");
  }

  if (overlay.interfaceName != network.interfaceName) {
    return issue(IntegrationStage::DeviceTree,
                 Severity::Critical,
                 "INTERFACE_NAME_MISMATCH",
                 "overlay interface name does not match network profile");
  }

  if (overlay.bus == BusType::Sdio && overlay.maxFrequencyHz < 25000000U) {
    return issue(IntegrationStage::DeviceTree,
                 Severity::Warning,
                 "SDIO_CLOCK_LOW",
                 "SDIO max-frequency is below expected bring-up target");
  }

  if (overlay.irqGpio.empty() || overlay.resetGpio.empty() || !overlay.powerSequencing) {
    return issue(IntegrationStage::DeviceTree,
                 Severity::Critical,
                 "GPIO_OR_POWER_SEQUENCE_MISSING",
                 "IRQ, reset GPIO, and power sequencing are required");
  }

  if (overlay.firmwarePath.empty()) {
    return issue(IntegrationStage::FirmwareLoad,
                 Severity::Critical,
                 "FIRMWARE_PATH_MISSING",
                 "device-tree overlay must name the firmware blob");
  }

  if (overlay.regulatoryDomain.size() != 2U) {
    return issue(IntegrationStage::DeviceTree,
                 Severity::Warning,
                 "REGDOMAIN_UNSET",
                 "regulatory domain should be a two-letter country code");
  }

  return std::nullopt;
}

std::string DeviceTreeRule::name() const {
  return "DeviceTreeRule";
}

std::optional<IntegrationIssue> ModuleLoadRule::evaluate(
    const KernelBuildConfig& build,
    const DeviceTreeOverlay&,
    const NetworkProfile&,
    const DriverObservation& observation) const {
  if (!observation.moduleInserted) {
    return issue(IntegrationStage::ModuleLoad,
                 Severity::Critical,
                 "MODULE_INSERT_FAILED",
                 "insmod/modprobe did not load " + build.moduleName);
  }

  if (!observation.firmwareLoaded) {
    return issue(IntegrationStage::FirmwareLoad,
                 Severity::Critical,
                 "FIRMWARE_LOAD_FAILED",
                 "driver did not report successful firmware load");
  }

  if (!observation.phyRegistered) {
    return issue(IntegrationStage::ModuleLoad,
                 Severity::Critical,
                 "WIPHY_NOT_REGISTERED",
                 "cfg80211 wiphy registration did not complete");
  }

  return std::nullopt;
}

std::string ModuleLoadRule::name() const {
  return "ModuleLoadRule";
}

std::optional<IntegrationIssue> NetworkAssociationRule::evaluate(
    const KernelBuildConfig&,
    const DeviceTreeOverlay&,
    const NetworkProfile& network,
    const DriverObservation& observation) const {
  if (!observation.interfaceUp) {
    return issue(IntegrationStage::InterfaceBringup,
                 Severity::Critical,
                 "INTERFACE_DOWN",
                 network.interfaceName + " is not administratively up");
  }

  if (!validCidr(network.addressCidr)) {
    return issue(IntegrationStage::InterfaceBringup,
                 Severity::Critical,
                 "IP_CONFIG_INVALID",
                 "network address must use CIDR notation");
  }

  if (!observation.associated) {
    return issue(IntegrationStage::Association,
                 Severity::Critical,
                 "WIFI_NOT_ASSOCIATED",
                 "station is not associated with SSID " + network.ssid);
  }

  if (observation.rssiDbm < network.minRssiDbm) {
    return issue(IntegrationStage::Association,
                 Severity::Warning,
                 "RSSI_BELOW_TARGET",
                 "RSSI " + std::to_string(observation.rssiDbm) +
                     " dBm is weaker than " + std::to_string(network.minRssiDbm) +
                     " dBm target");
  }

  if (network.mtu < 1280U || network.mtu > 2304U) {
    return issue(IntegrationStage::InterfaceBringup,
                 Severity::Warning,
                 "MTU_OUTSIDE_WIFI_RANGE",
                 "MTU should stay inside practical Wi-Fi test bounds");
  }

  return std::nullopt;
}

std::string NetworkAssociationRule::name() const {
  return "NetworkAssociationRule";
}

std::optional<IntegrationIssue> SocketTrafficRule::evaluate(
    const KernelBuildConfig&,
    const DeviceTreeOverlay&,
    const NetworkProfile& network,
    const DriverObservation& observation) const {
  if (network.hostPeer.empty() || network.hostPort == 0U) {
    return issue(IntegrationStage::SocketTraffic,
                 Severity::Critical,
                 "HOST_PEER_MISSING",
                 "host PC endpoint and port are required for socket validation");
  }

  if (!observation.socketEchoMatched) {
    return issue(IntegrationStage::SocketTraffic,
                 Severity::Critical,
                 "SOCKET_ECHO_FAILED",
                 "host PC did not echo the expected payload");
  }

  if (observation.socketBytesSent == 0U ||
      observation.socketBytesSent != observation.socketBytesReceived) {
    return issue(IntegrationStage::SocketTraffic,
                 Severity::Critical,
                 "SOCKET_BYTE_COUNT_MISMATCH",
                 "sent and received socket byte counts do not match");
  }

  if (observation.txPackets == 0U || observation.rxPackets == 0U) {
    return issue(IntegrationStage::SocketTraffic,
                 Severity::Critical,
                 "PACKET_COUNTERS_EMPTY",
                 "interface packet counters did not increase during test");
  }

  return std::nullopt;
}

std::string SocketTrafficRule::name() const {
  return "SocketTrafficRule";
}

void CompositeRuleSet::add(std::unique_ptr<IIntegrationRule> rule) {
  if (!rule) {
    throw std::invalid_argument("integration rule cannot be null");
  }
  rules_.push_back(std::move(rule));
}

std::vector<IntegrationIssue> CompositeRuleSet::evaluate(
    const KernelBuildConfig& build,
    const DeviceTreeOverlay& overlay,
    const NetworkProfile& network,
    const DriverObservation& observation) const {
  std::vector<IntegrationIssue> issues;
  for (const auto& rule : rules_) {
    if (const auto result = rule->evaluate(build, overlay, network, observation)) {
      issues.push_back(*result);
    }
  }
  return issues;
}

std::vector<std::string> CompositeRuleSet::trace() const {
  std::vector<std::string> names;
  for (const auto& rule : rules_) {
    names.push_back(rule->name());
  }
  return names;
}

std::string LinuxCommandPlanBuilder::buildPlan(
    const KernelBuildConfig& build,
    const DeviceTreeOverlay& overlay,
    const NetworkProfile& network) const {
  std::ostringstream plan;
  plan << "make -C " << quote(build.kernelSourcePath)
       << " M=" << quote(build.moduleSourcePath)
       << " ARCH=" << build.architecture
       << " CROSS_COMPILE=" << build.crossCompilerPrefix
       << (build.warningsAsErrors ? " WERROR=1" : "")
       << " modules\n"
       << "dtc -@ -I dts -O dtb -o " << overlay.overlayName << ".dtbo "
       << overlay.overlayName << ".dts\n"
       << "scp " << build.moduleName << ".ko " << overlay.firmwarePath
       << " root@target:/lib/firmware/\n"
       << "modprobe cfg80211 && insmod " << build.moduleName << ".ko\n"
       << "ip link set " << network.interfaceName << " up\n"
       << "wpa_supplicant -B -i " << network.interfaceName
       << " -c /etc/wpa_supplicant/" << network.interfaceName << ".conf\n"
       << "ip addr add " << network.addressCidr << " dev "
       << network.interfaceName << "\n"
       << "iperf3 -c " << network.hostPeer << " -p " << network.hostPort;
  return plan.str();
}

std::string JsonTelemetryEncoder::encode(const IntegrationReport& report) const {
  std::ostringstream payload;
  payload << "{"
          << "\"module\":\"" << jsonEscape(report.build.moduleName) << "\","
          << "\"kernel\":\"" << jsonEscape(report.build.kernelRelease) << "\","
          << "\"interface\":\"" << jsonEscape(report.network.interfaceName) << "\","
          << "\"bus\":\"" << toString(report.overlay.bus) << "\","
          << "\"ssid\":\"" << jsonEscape(report.network.ssid) << "\","
          << "\"accepted\":" << (report.accepted ? "true" : "false") << ","
          << "\"rssiDbm\":" << static_cast<int>(report.observation.rssiDbm) << ","
          << "\"linkMbps\":" << fixed2(report.observation.linkMbps) << ","
          << "\"socketBytesSent\":" << report.observation.socketBytesSent << ","
          << "\"socketBytesReceived\":" << report.observation.socketBytesReceived << ","
          << "\"issueCount\":" << report.issues.size() << "}";
  return payload.str();
}

WifiIntegrationRunner::WifiIntegrationRunner(
    KernelBuildConfig build,
    DeviceTreeOverlay overlay,
    NetworkProfile network,
    DriverObservation observation,
    CompositeRuleSet rules,
    const ICommandPlanBuilder& commandPlanBuilder,
    const ITelemetryEncoder& telemetryEncoder)
    : build_(std::move(build)),
      overlay_(std::move(overlay)),
      network_(std::move(network)),
      observation_(std::move(observation)),
      rules_(std::move(rules)),
      commandPlanBuilder_(commandPlanBuilder),
      telemetryEncoder_(telemetryEncoder) {}

IntegrationReport WifiIntegrationRunner::run() const {
  IntegrationReport report;
  report.build = build_;
  report.overlay = overlay_;
  report.network = network_;
  report.observation = observation_;
  report.ruleTrace = rules_.trace();
  report.issues = rules_.evaluate(build_, overlay_, network_, observation_);
  report.accepted = std::none_of(
      report.issues.begin(),
      report.issues.end(),
      [](const IntegrationIssue& item) {
        return item.severity == Severity::Critical;
      });
  report.commandPlan = commandPlanBuilder_.buildPlan(build_, overlay_, network_);
  report.telemetryPayload = telemetryEncoder_.encode(report);
  return report;
}

TextReportWriter::TextReportWriter(std::ostream& stream) : stream_(stream) {}

void TextReportWriter::write(const IntegrationReport& report) const {
  stream_ << "wifi_integration=" << (report.accepted ? "PASS" : "FAIL") << '\n'
          << "module=" << report.build.moduleName << '\n'
          << "kernel=" << report.build.kernelRelease << '\n'
          << "target=" << report.build.architecture << " cross="
          << report.build.crossCompilerPrefix << '\n'
          << "overlay=" << report.overlay.overlayName
          << " compatible=" << report.overlay.compatible
          << " bus=" << toString(report.overlay.bus) << '\n'
          << "interface=" << report.network.interfaceName
          << " ssid=" << report.network.ssid
          << " auth=" << toString(report.network.authMode) << '\n'
          << "rssi_dbm=" << static_cast<int>(report.observation.rssiDbm)
          << " link_mbps=" << fixed2(report.observation.linkMbps)
          << " socket_bytes=" << report.observation.socketBytesReceived << '\n';

  for (const auto& rule : report.ruleTrace) {
    stream_ << "rule=" << rule << '\n';
  }

  for (const auto& item : report.issues) {
    stream_ << "issue=" << toString(item.severity) << ':'
            << toString(item.stage) << ':' << item.code << ':'
            << item.detail << '\n';
  }

  stream_ << "command_plan_begin\n" << report.commandPlan
          << "\ncommand_plan_end\n"
          << "telemetry=" << report.telemetryPayload << '\n';
}

KernelBuildConfig demoBuildConfig() {
  return KernelBuildConfig{
      "rheslar_wifi",
      "6.6.32-rt-lab",
      "arm",
      "arm-linux-gnueabihf-",
      "/opt/linux-6.6.32",
      "/work/drivers/rheslar_wifi",
      true,
      {"rheslar_wifi_probe", "rheslar_wifi_remove", "rheslar_wifi_xmit"}};
}

DeviceTreeOverlay demoOverlay() {
  return DeviceTreeOverlay{
      "rheslar-wifi0",
      "rheslar,wifi-sdio-lab",
      BusType::Sdio,
      0U,
      50000000U,
      "gpio1_18",
      "gpio1_19",
      "/lib/firmware/rheslar_wifi/fw.bin",
      "US",
      "wlan0",
      true};
}

NetworkProfile demoNetwork() {
  return NetworkProfile{
      "wlan0",
      "RheslarLab",
      AuthMode::Wpa2Psk,
      "192.168.50.44/24",
      "192.168.50.1",
      "192.168.50.1",
      -72,
      1500U,
      6U,
      "192.168.50.10",
      5001U};
}

DriverObservation nominalObservation() {
  return DriverObservation{
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      -55,
      72.2,
      402U,
      398U,
      4096U,
      4096U,
      {"rheslar_wifi_probe", "rheslar_wifi_remove", "rheslar_wifi_xmit"}};
}

DriverObservation missingFirmwareObservation() {
  auto observation = nominalObservation();
  observation.firmwareLoaded = false;
  observation.phyRegistered = false;
  return observation;
}

DriverObservation weakLinkObservation() {
  auto observation = nominalObservation();
  observation.rssiDbm = -84;
  observation.linkMbps = 5.5;
  return observation;
}

DriverObservation socketFailureObservation() {
  auto observation = nominalObservation();
  observation.socketEchoMatched = false;
  observation.socketBytesReceived = 1024U;
  return observation;
}

CompositeRuleSet defaultRules() {
  CompositeRuleSet rules;
  rules.add(std::make_unique<BuildConfigurationRule>());
  rules.add(std::make_unique<DeviceTreeRule>());
  rules.add(std::make_unique<ModuleLoadRule>());
  rules.add(std::make_unique<NetworkAssociationRule>());
  rules.add(std::make_unique<SocketTrafficRule>());
  return rules;
}

IntegrationReport runScenario(const std::string& scenario) {
  auto build = demoBuildConfig();
  auto overlay = demoOverlay();
  auto network = demoNetwork();
  auto observation = nominalObservation();

  if (scenario == "missing-firmware") {
    observation = missingFirmwareObservation();
  } else if (scenario == "bad-dts") {
    overlay.compatible = "vendor,unknown-wifi";
  } else if (scenario == "weak-link") {
    observation = weakLinkObservation();
  } else if (scenario == "socket-fail") {
    observation = socketFailureObservation();
  } else if (scenario == "bad-cross") {
    build.crossCompilerPrefix = "x86_64-linux-gnu-";
  } else if (scenario != "nominal") {
    throw std::invalid_argument("unknown scenario: " + scenario);
  }

  const LinuxCommandPlanBuilder commandPlanBuilder;
  const JsonTelemetryEncoder telemetryEncoder;
  const WifiIntegrationRunner runner(
      build,
      overlay,
      network,
      observation,
      defaultRules(),
      commandPlanBuilder,
      telemetryEncoder);
  return runner.run();
}

}  // namespace wifi_integration
