/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cerrno>
#include <cstdlib>
#include <optional>

namespace sndlib {

// Accepted range for -sdlSndSize, in sample frames. SDL treats the value as a
// request only; the device may round or ignore it.
constexpr int kMinSoundBufferFrames = 64;
constexpr int kMaxSoundBufferFrames = 8192;

// Parses a requested device buffer size. Returns no value for a missing,
// malformed (including trailing characters), or out-of-range argument.
inline std::optional<int> ParseSoundBufferFrames(const char *text) {
  if (text == nullptr || *text < '0' || *text > '9')
    return std::nullopt;
  char *end = nullptr;
  errno = 0;
  const long value = std::strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value < kMinSoundBufferFrames || value > kMaxSoundBufferFrames)
    return std::nullopt;
  return static_cast<int>(value);
}

} // namespace sndlib
