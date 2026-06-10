#include "wifi_integration/WifiIntegration.hpp"

#include <cassert>
#include <sstream>
#include <string>

namespace {

bool contains(const std::string& value, const std::string& needle) {
  return value.find(needle) != std::string::npos;
}

bool hasIssue(const wifi_integration::IntegrationReport& report,
              const std::string& code) {
  for (const auto& issue : report.issues) {
    if (issue.code == code) {
      return true;
    }
  }
  return false;
}

void nominalScenarioPasses() {
  const auto report = wifi_integration::runScenario("nominal");

  assert(report.accepted);
  assert(report.issues.empty());
  assert(report.ruleTrace.size() == 5U);
  assert(contains(report.commandPlan, "make -C"));
  assert(contains(report.commandPlan, "iperf3"));
  assert(contains(report.telemetryPayload, "\"accepted\":true"));
  assert(report.observation.socketBytesSent == report.observation.socketBytesReceived);
}

void badCrossCompilerFailsBuildGate() {
  const auto report = wifi_integration::runScenario("bad-cross");

  assert(!report.accepted);
  assert(hasIssue(report, "CROSS_COMPILE_UNSUPPORTED"));
}

void badDeviceTreeFailsOverlayGate() {
  const auto report = wifi_integration::runScenario("bad-dts");

  assert(!report.accepted);
  assert(hasIssue(report, "COMPATIBLE_MISMATCH"));
}

void missingFirmwareFailsModuleGate() {
  const auto report = wifi_integration::runScenario("missing-firmware");

  assert(!report.accepted);
  assert(hasIssue(report, "FIRMWARE_LOAD_FAILED"));
}

void weakLinkIsWarningOnly() {
  const auto report = wifi_integration::runScenario("weak-link");

  assert(report.accepted);
  assert(hasIssue(report, "RSSI_BELOW_TARGET"));
  assert(contains(report.telemetryPayload, "\"issueCount\":1"));
}

void socketFailureIsCritical() {
  const auto report = wifi_integration::runScenario("socket-fail");

  assert(!report.accepted);
  assert(hasIssue(report, "SOCKET_ECHO_FAILED"));
}

void textReportIncludesEvidence() {
  const auto report = wifi_integration::runScenario("nominal");
  std::ostringstream output;
  wifi_integration::TextReportWriter writer(output);

  writer.write(report);

  assert(contains(output.str(), "wifi_integration=PASS"));
  assert(contains(output.str(), "rule=BuildConfigurationRule"));
  assert(contains(output.str(), "command_plan_begin"));
  assert(contains(output.str(), "telemetry="));
}

}  // namespace

int main() {
  nominalScenarioPasses();
  badCrossCompilerFailsBuildGate();
  badDeviceTreeFailsOverlayGate();
  missingFirmwareFailsModuleGate();
  weakLinkIsWarningOnly();
  socketFailureIsCritical();
  textReportIncludesEvidence();
  return 0;
}
