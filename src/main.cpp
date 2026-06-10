#include "wifi_integration/WifiIntegration.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage(const char* programName) {
  std::cout << "Usage: " << programName << " [scenario]\n\n"
            << "Scenarios:\n"
            << "  --nominal           Accepted driver build and network integration\n"
            << "  --missing-firmware  Firmware load failure after module insertion\n"
            << "  --bad-dts           Device-tree compatible mismatch\n"
            << "  --weak-link         Associated interface with weak RSSI warning\n"
            << "  --socket-fail       Host socket echo validation failure\n"
            << "  --bad-cross         Unsupported cross compiler prefix\n"
            << "  --help              Show this help text\n";
}

std::string scenarioFromOption(const std::string& option) {
  if (option == "--nominal") {
    return "nominal";
  }
  if (option == "--missing-firmware") {
    return "missing-firmware";
  }
  if (option == "--bad-dts") {
    return "bad-dts";
  }
  if (option == "--weak-link") {
    return "weak-link";
  }
  if (option == "--socket-fail") {
    return "socket-fail";
  }
  if (option == "--bad-cross") {
    return "bad-cross";
  }
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  const std::string option = argc > 1 ? argv[1] : "--nominal";
  if (option == "--help") {
    printUsage(argv[0]);
    return 0;
  }

  const std::string scenario = scenarioFromOption(option);
  if (scenario.empty()) {
    printUsage(argv[0]);
    return 1;
  }

  try {
    const auto report = wifi_integration::runScenario(scenario);
    wifi_integration::TextReportWriter writer(std::cout);
    writer.write(report);
    return report.accepted ? 0 : 2;
  } catch (const std::exception& exception) {
    std::cerr << "wifi integration error: " << exception.what() << '\n';
    return 1;
  }
}
