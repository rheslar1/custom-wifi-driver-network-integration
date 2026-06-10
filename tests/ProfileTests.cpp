#include <array>
#include <cassert>
#include <string_view>

class IReadinessRule {
 public:
  virtual ~IReadinessRule() = default;
  virtual bool passes(std::string_view evidenceTarget) const = 0;
};

class RequiredEvidenceRule final : public IReadinessRule {
 public:
  bool passes(std::string_view evidenceTarget) const override {
    return !evidenceTarget.empty();
  }
};

struct ProjectProfile {
  std::string_view title;
  std::string_view summary;
  std::string_view evidenceTarget;
  std::array<std::string_view, 9> tags;
};

constexpr ProjectProfile profile{
  "Custom Wi-Fi Driver Compilation & Network Integration",
  "Kernel-module and network-stack project that cross-compiles Wi-Fi driver support, updates device tree integration, and validates socket traffic with a host PC.",
  "Kernel module workflow, network interface bring-up, device-tree awareness, and user-space C networking validation.",
  {
    "C++17",
    "C++ Design Patterns",
    "SOLID",
    "Linux kernel",
    "Wi-Fi driver",
    "Kernel module",
    "Device tree",
    "Cross-compilation",
    "Sockets"
  }
};

int main() {
  const RequiredEvidenceRule rule;
  assert(!profile.title.empty());
  assert(!profile.summary.empty());
  assert(rule.passes(profile.evidenceTarget));
  assert(profile.tags[0] == "C++17");
  return 0;
}
