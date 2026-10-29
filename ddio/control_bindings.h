#ifndef D3_CONTROL_BINDINGS_H
#define D3_CONTROL_BINDINGS_H

#include <array>
#include <cstddef>
#include <vector>
#include "controller.h"

namespace control_bindings {
struct Input {
  ct_type type = ctNone;
  uint8_t controller = NULL_CONTROLLER;
  uint8_t value = NULL_BINDING;
  uint8_t flags = 0;

  bool bound() const { return type != ctNone && controller != NULL_CONTROLLER && value != NULL_BINDING; }
  bool same_input(const Input &other) const {
    return bound() && other.bound() && type == other.type && value == other.value &&
           (type == ctKey || controller == other.controller);
  }
};

struct Function {
  int id;
  std::array<Input, CTLBINDS_PER_FUNC> inputs;
};

struct Conflict { size_t function; size_t slot; };
enum class Resolution { Cancel, Replace, Swap };

inline std::vector<Conflict> conflicts(const std::vector<Function> &functions, int id, int, const Input &input) {
  std::vector<Conflict> result;
  for (size_t f = 0; f < functions.size(); ++f) {
    for (size_t s = 0; s < CTLBINDS_PER_FUNC; ++s) {
      if (functions[f].id == id) // moving between the same action's two slots is harmless
        continue;
      if (functions[f].inputs[s].same_input(input))
        result.push_back({f, s});
    }
  }
  return result;
}

// Prepare the entire edit before touching the live controller. Cancel and an
// ambiguous swap leave every binding unchanged.
inline bool assign(std::vector<Function> &functions, int id, int slot, Input input, Resolution resolution) {
  if (resolution == Resolution::Cancel || slot < 0 || slot >= CTLBINDS_PER_FUNC || !input.bound())
    return false;
  Function *target = nullptr;
  for (auto &function : functions)
    if (function.id == id)
      target = &function;
  if (!target)
    return false;
  const auto matches = conflicts(functions, id, slot, input);
  if (resolution == Resolution::Swap && matches.size() != 1)
    return false;
  const Input old = target->inputs[slot];
  for (const auto &match : matches) {
    auto &other = functions[match.function].inputs[match.slot];
    other = resolution == Resolution::Swap ? old : Input{};
  }
  for (auto &existing : target->inputs)
    if (existing.same_input(input))
      existing = Input{};
  input.flags = 0; // a newly assigned input must not inherit axis inversion
  target->inputs[slot] = input;
  return true;
}
} // namespace control_bindings
#endif
