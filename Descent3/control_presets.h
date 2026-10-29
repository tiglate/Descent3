#ifndef D3_CONTROL_PRESETS_H
#define D3_CONTROL_PRESETS_H

#include "control_functions.h"
#include "controller.h"
#include "ddio_common.h"

enum class ControlPreset { Modern, Classic, Custom };

// Start with the common controller defaults. Only keyboard/mouse controls
// belong to a preset; joystick bindings and sensitivity remain independent.
inline ct_function ControlPresetFunction(ct_function function, ControlPreset preset) {
  if (preset == ControlPreset::Custom)
    return function;
  const bool modern = preset == ControlPreset::Modern;
  switch (function.id) {
  case ctfFORWARD_THRUSTKEY: function.value[0] = modern ? KEY_W : KEY_A; break;
  case ctfREVERSE_THRUSTKEY: function.value[0] = modern ? KEY_S : KEY_Z; break;
  case ctfLEFT_THRUSTKEY: function.value[0] = modern ? KEY_A : KEY_PAD1; break;
  case ctfRIGHT_THRUSTKEY: function.value[0] = modern ? KEY_D : KEY_PAD3; break;
  case ctfUP_THRUSTKEY: function.value[0] = modern ? KEY_SPACEBAR : KEY_PADMINUS; break;
  case ctfDOWN_THRUSTKEY: function.value[0] = modern ? KEY_LCTRL : KEY_PADPLUS; break;
  case ctfAFTERBURN_KEY: function.value[0] = modern ? KEY_LSHIFT : KEY_S; break;
  case ctfFIREPRIMARY_KEY:
    function.value[0] = modern ? 0 : KEY_LCTRL;
    function.value[1] = modern ? 0 : KEY_RCTRL;
    break;
  case ctfFIRESECONDARY_KEY: function.value[0] = modern ? 0 : KEY_SPACEBAR; break;
  default: break;
  }
  return function;
}

inline bool IsPresetInput(ct_type type) {
  return type == ctKey || type == ctMouseButton || type == ctMouseAxis;
}
#endif
