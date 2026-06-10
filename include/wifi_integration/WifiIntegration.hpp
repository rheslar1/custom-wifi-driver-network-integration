#ifndef WIFI_INTEGRATION_WIFI_INTEGRATION_HPP_
#define WIFI_INTEGRATION_WIFI_INTEGRATION_HPP_

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace wifi_integration {

enum class BusType {
  Sdio,
  Spi,
  Pcie,
  Usb
};

enum class AuthMode {
  Open,
  Wpa2Psk,
  Wpa3Sae,
  Enterprise
};

enum class Severity {
  Info,
  Warning,
  Critical
};

enum class IntegrationStage {
  Build,
  DeviceTree,
  ModuleLoad,
  FirmwareLoad,
  InterfaceBringup,
  Association,
  SocketTraffic
};

std::string toString(BusType bus);
std::string toString(AuthMode mode);
std::string toString(Severity severity);
std::string toString(IntegrationStage stage);

struct KernelBuildConfig {
  std::string moduleName;
  std::string kernelRelease;
  std::string architecture;
  std::string crossCompilerPrefix;
  std::string kernelSourcePath;
  std::string moduleSourcePath;
  bool warningsAsErrors{true};
  std::vector<std::string> expectedSymbols;
};

struct DeviceTreeOverlay {
  std::string overlayName;
  std::string compatible;
  BusType bus{BusType::Sdio};
  std::uint8_t chipSelect{};
  std::uint32_t maxFrequencyHz{};
  std::string irqGpio;
  std::string resetGpio;
  std::string firmwarePath;
  std::string regulatoryDomain;
  std::string interfaceName;
  bool powerSequencing{};
};

struct NetworkProfile {
  std::string interfaceName;
  std::string ssid;
  AuthMode authMode{AuthMode::Wpa2Psk};
  std::string addressCidr;
  std::string gateway;
  std::string dns;
  std::int8_t minRssiDbm{-72};
  std::uint16_t mtu{1500};
  std::uint8_t channel{6};
  std::string hostPeer;
  std::uint16_t hostPort{5001};
};

struct DriverObservation {
  bool moduleBuilt{};
  bool moduleInserted{};
  bool firmwareLoaded{};
  bool phyRegistered{};
  bool interfaceUp{};
  bool associated{};
  bool socketEchoMatched{};
  std::int8_t rssiDbm{-127};
  double linkMbps{};
  std::uint32_t txPackets{};
  std::uint32_t rxPackets{};
  std::uint32_t socketBytesSent{};
  std::uint32_t socketBytesReceived{};
  std::vector<std::string> exportedSymbols;
};

struct IntegrationIssue {
  IntegrationStage stage{IntegrationStage::Build};
  Severity severity{Severity::Critical};
  std::string code;
  std::string detail;
};

struct IntegrationReport {
  bool accepted{};
  KernelBuildConfig build;
  DeviceTreeOverlay overlay;
  NetworkProfile network;
  DriverObservation observation;
  std::vector<std::string> ruleTrace;
  std::vector<IntegrationIssue> issues;
  std::string commandPlan;
  std::string telemetryPayload;
};

class IIntegrationRule {
 public:
  virtual ~IIntegrationRule() = default;
  virtual std::optional<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const = 0;
  virtual std::string name() const = 0;
};

class ICommandPlanBuilder {
 public:
  virtual ~ICommandPlanBuilder() = default;
  virtual std::string buildPlan(const KernelBuildConfig& build,
                                const DeviceTreeOverlay& overlay,
                                const NetworkProfile& network) const = 0;
};

class ITelemetryEncoder {
 public:
  virtual ~ITelemetryEncoder() = default;
  virtual std::string encode(const IntegrationReport& report) const = 0;
};

class BuildConfigurationRule final : public IIntegrationRule {
 public:
  std::optional<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const override;
  std::string name() const override;
};

class DeviceTreeRule final : public IIntegrationRule {
 public:
  std::optional<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const override;
  std::string name() const override;
};

class ModuleLoadRule final : public IIntegrationRule {
 public:
  std::optional<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const override;
  std::string name() const override;
};

class NetworkAssociationRule final : public IIntegrationRule {
 public:
  std::optional<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const override;
  std::string name() const override;
};

class SocketTrafficRule final : public IIntegrationRule {
 public:
  std::optional<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const override;
  std::string name() const override;
};

class CompositeRuleSet final {
 public:
  void add(std::unique_ptr<IIntegrationRule> rule);
  std::vector<IntegrationIssue> evaluate(
      const KernelBuildConfig& build,
      const DeviceTreeOverlay& overlay,
      const NetworkProfile& network,
      const DriverObservation& observation) const;
  std::vector<std::string> trace() const;

 private:
  std::vector<std::unique_ptr<IIntegrationRule>> rules_;
};

class LinuxCommandPlanBuilder final : public ICommandPlanBuilder {
 public:
  std::string buildPlan(const KernelBuildConfig& build,
                        const DeviceTreeOverlay& overlay,
                        const NetworkProfile& network) const override;
};

class JsonTelemetryEncoder final : public ITelemetryEncoder {
 public:
  std::string encode(const IntegrationReport& report) const override;
};

class WifiIntegrationRunner final {
 public:
  WifiIntegrationRunner(KernelBuildConfig build,
                        DeviceTreeOverlay overlay,
                        NetworkProfile network,
                        DriverObservation observation,
                        CompositeRuleSet rules,
                        const ICommandPlanBuilder& commandPlanBuilder,
                        const ITelemetryEncoder& telemetryEncoder);

  IntegrationReport run() const;

 private:
  KernelBuildConfig build_;
  DeviceTreeOverlay overlay_;
  NetworkProfile network_;
  DriverObservation observation_;
  CompositeRuleSet rules_;
  const ICommandPlanBuilder& commandPlanBuilder_;
  const ITelemetryEncoder& telemetryEncoder_;
};

class TextReportWriter final {
 public:
  explicit TextReportWriter(std::ostream& stream);
  void write(const IntegrationReport& report) const;

 private:
  std::ostream& stream_;
};

KernelBuildConfig demoBuildConfig();
DeviceTreeOverlay demoOverlay();
NetworkProfile demoNetwork();
DriverObservation nominalObservation();
DriverObservation missingFirmwareObservation();
DriverObservation weakLinkObservation();
DriverObservation socketFailureObservation();
CompositeRuleSet defaultRules();
IntegrationReport runScenario(const std::string& scenario);

}  // namespace wifi_integration

#endif  // WIFI_INTEGRATION_WIFI_INTEGRATION_HPP_
