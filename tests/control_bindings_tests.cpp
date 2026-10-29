#include <algorithm>
#include <array>
#include <gtest/gtest.h>

#include "control_bindings.h"
#include "control_presets.h"
#include "joystick.h"
#include "mouse_buttons.h"

namespace {
using namespace control_bindings;
const std::array<ct_function, NUM_CONTROLLER_FUNCTIONS> defaults = {{
#include "control_defaults.inc"
}};

const ct_function &need(int id) {
  return *std::find_if(defaults.begin(), defaults.end(), [id](const auto &fn) { return fn.id == id; });
}

Input key(uint8_t value) { return {ctKey, 0, value, 0}; }
std::vector<Function> bindings() {
  return {{1, {key(KEY_W), key(KEY_UP)}}, {2, {key(KEY_S), Input{}}},
          {3, {Input{ctMouseButton, 1, 1, 0}, Input{}}}};
}
}

TEST(ControlPresets, ModernDefaultsHaveAllMovementBindingsAndNoFireConflicts) {
  EXPECT_EQ(need(ctfFORWARD_THRUSTKEY).value[0], KEY_W);
  EXPECT_EQ(need(ctfREVERSE_THRUSTKEY).value[0], KEY_S);
  EXPECT_EQ(need(ctfLEFT_THRUSTKEY).value[0], KEY_A);
  EXPECT_EQ(need(ctfRIGHT_THRUSTKEY).value[0], KEY_D);
  EXPECT_EQ(need(ctfUP_THRUSTKEY).value[0], KEY_SPACEBAR);
  EXPECT_EQ(need(ctfDOWN_THRUSTKEY).value[0], KEY_LCTRL);
  EXPECT_EQ(need(ctfAFTERBURN_KEY).value[0], KEY_LSHIFT);
  EXPECT_EQ(need(ctfFIREPRIMARY_BUTTON).value[1], 1);
  EXPECT_EQ(need(ctfFIRESECONDARY_BUTTON).value[1], 2);
  EXPECT_EQ(need(ctfFIREFLARE_BUTTON).value[1], 3);
  EXPECT_NE(need(ctfPITCH_DOWNAXIS).flags[1] & CTFNF_INVERT, 0);
  for (auto id : {ctfFIREPRIMARY_KEY, ctfFIREPRIMARY_KEY2, ctfFIRESECONDARY_KEY})
    for (auto value : need(id).value)
      EXPECT_EQ(value, 0);
}

TEST(ControlPresets, EveryDefaultFunctionHasAUniqueStableId) {
  std::array<bool, NUM_CONTROLLER_FUNCTIONS> seen{};
  for (const auto &fn : defaults) {
    ASSERT_GE(fn.id, 0);
    ASSERT_LT(fn.id, NUM_CONTROLLER_FUNCTIONS);
    EXPECT_FALSE(seen[fn.id]);
    seen[fn.id] = true;
  }
}

TEST(ControlPresets, KeyboardBindingsDoNotOverlap) {
  for (auto preset : {ControlPreset::Modern, ControlPreset::Classic}) {
    std::array<bool, DDIO_MAX_KEYS> seen{};
    for (const auto &base : defaults) {
      const auto fn = ControlPresetFunction(base, preset);
      for (int slot = 0; slot < CTLBINDS_PER_FUNC; ++slot) {
        if (fn.ctype[slot] == ctKey && fn.value[slot]) {
          EXPECT_FALSE(seen[fn.value[slot]]) << "preset " << static_cast<int>(preset) << " action " << fn.id;
          seen[fn.value[slot]] = true;
        }
      }
    }
  }
}

TEST(ControlPresets, ClassicRoundTripsBackToModern) {
  EXPECT_EQ(ControlPresetFunction(need(ctfFORWARD_THRUSTKEY), ControlPreset::Classic).value[0], KEY_A);
  EXPECT_EQ(ControlPresetFunction(need(ctfUP_THRUSTKEY), ControlPreset::Classic).value[0], KEY_PADMINUS);
  EXPECT_EQ(ControlPresetFunction(need(ctfFIREPRIMARY_KEY), ControlPreset::Classic).value[0], KEY_LCTRL);
  EXPECT_EQ(ControlPresetFunction(need(ctfFIRESECONDARY_KEY), ControlPreset::Classic).value[0], KEY_SPACEBAR);
  for (const auto &base : defaults) {
    const auto restored = ControlPresetFunction(ControlPresetFunction(base, ControlPreset::Classic), ControlPreset::Modern);
    for (int slot = 0; slot < CTLBINDS_PER_FUNC; ++slot) {
      EXPECT_EQ(restored.value[slot], base.value[slot]);
      EXPECT_EQ(restored.ctype[slot], base.ctype[slot]);
      EXPECT_EQ(restored.flags[slot], base.flags[slot]);
    }
  }
}

TEST(ControlBindings, CancelLeavesAllInputsUnchanged) {
  auto functions = bindings();
  EXPECT_FALSE(assign(functions, 1, 0, key(KEY_S), Resolution::Cancel));
  EXPECT_EQ(functions[0].inputs[0].value, KEY_W);
  EXPECT_EQ(functions[1].inputs[0].value, KEY_S);
}

TEST(ControlBindings, ReplaceClearsOnlyTheConflictingInput) {
  auto functions = bindings();
  EXPECT_TRUE(assign(functions, 1, 0, key(KEY_S), Resolution::Replace));
  EXPECT_EQ(functions[0].inputs[0].value, KEY_S);
  EXPECT_EQ(functions[0].inputs[1].value, KEY_UP);
  EXPECT_FALSE(functions[1].inputs[0].bound());
  EXPECT_TRUE(functions[2].inputs[0].bound());
}

TEST(ControlBindings, SwapPreservesBothActions) {
  auto functions = bindings();
  EXPECT_TRUE(assign(functions, 1, 0, key(KEY_S), Resolution::Swap));
  EXPECT_EQ(functions[0].inputs[0].value, KEY_S);
  EXPECT_EQ(functions[1].inputs[0].value, KEY_W);
}

TEST(ControlBindings, MultipleConflictsRequireReplaceOrCancel) {
  auto functions = bindings();
  functions[2].inputs[1] = key(KEY_S);
  EXPECT_EQ(conflicts(functions, 1, 0, key(KEY_S)).size(), 2);
  EXPECT_FALSE(assign(functions, 1, 0, key(KEY_S), Resolution::Swap));
  EXPECT_EQ(functions[0].inputs[0].value, KEY_W);
  EXPECT_TRUE(assign(functions, 1, 0, key(KEY_S), Resolution::Replace));
  EXPECT_FALSE(functions[1].inputs[0].bound());
  EXPECT_FALSE(functions[2].inputs[1].bound());
}

TEST(ControlBindings, MovingAnAlternateWithinSameActionDoesNotPromptOrDuplicate) {
  auto functions = bindings();
  EXPECT_TRUE(conflicts(functions, 1, 0, key(KEY_UP)).empty());
  EXPECT_TRUE(assign(functions, 1, 0, key(KEY_UP), Resolution::Replace));
  EXPECT_EQ(functions[0].inputs[0].value, KEY_UP);
  EXPECT_FALSE(functions[0].inputs[1].bound());
}

TEST(ControlBindings, DevicesAndInputTypesAreDistinct) {
  auto functions = bindings();
  EXPECT_TRUE(conflicts(functions, 1, 0, {ctButton, 2, 1, 0}).empty());
  EXPECT_EQ(conflicts(functions, 1, 0, {ctMouseButton, 1, 1, 0}).size(), 1);
  EXPECT_TRUE(conflicts(functions, 1, 0, {ctMouseButton, 2, 1, 0}).empty());
}

TEST(ControlBindings, RebindingAnAxisClearsItsOldInversion) {
  auto functions = bindings();
  functions[0].inputs[0] = {ctMouseAxis, 1, CT_Y_AXIS, CTFNF_INVERT};
  EXPECT_TRUE(assign(functions, 1, 0, {ctMouseAxis, 1, CT_X_AXIS, 0}, Resolution::Replace));
  EXPECT_EQ(functions[0].inputs[0].flags, 0);
}

TEST(ControlBindings, PhysicalMouseButtonsDoNotOverlapWheelSlots) {
  EXPECT_EQ(EngineMouseButtonIndex(1), 0);
  EXPECT_EQ(EngineMouseButtonIndex(2), 2);
  EXPECT_EQ(EngineMouseButtonIndex(3), 1);
  std::array<bool, 8> used{};
  for (unsigned button = 1; button <= 6; ++button) {
    const int index = EngineMouseButtonIndex(button);
    ASSERT_GE(index, 0);
    ASSERT_LT(index, 8);
    EXPECT_NE(index, 4);
    EXPECT_NE(index, 5);
    EXPECT_FALSE(used[index]);
    used[index] = true;
  }
  EXPECT_EQ(EngineMouseButtonIndex(0), -1);
  EXPECT_EQ(EngineMouseButtonIndex(7), -1);
}
