/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <optional>

// Runtime timing policy for the reliable UDP layer, in seconds. Defaults match
// the historical compile-time constants (NETTIMEOUT, NETRETRYTIME,
// MIN_NET_RETRYTIME); packet layouts and the heartbeat interval are unchanged.
struct nw_timing_policy {
  float connect_timeout = 300.0f; // waiting for a connection handshake to complete
  float idle_timeout = 300.0f;    // dropping an established peer after silence
  float retry_time = 0.75f;       // base resend interval for unacknowledged packets
  float min_retry_time = 0.2f;    // lower bound for ping-derived resend intervals
};

namespace nw_timing {

constexpr float kHeartbeatSeconds = 10.0f; // NETHEARTBEATTIME
constexpr float kMinConnectTimeout = 1.0f;
constexpr float kMaxConnectTimeout = 600.0f;
// An idle limit below three heartbeats would drop healthy peers that are only
// sending heartbeats.
constexpr float kMinIdleTimeout = 3 * kHeartbeatSeconds;
constexpr float kMaxIdleTimeout = 3600.0f;
constexpr float kMinRetryTime = 0.05f;
constexpr float kMaxRetryTime = 5.0f;

// Returns nullptr when valid, otherwise a short reason.
inline const char *Validate(const nw_timing_policy &p) {
  auto in = [](float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; };
  if (!in(p.connect_timeout, kMinConnectTimeout, kMaxConnectTimeout))
    return "connect timeout must be between 1 and 600 seconds";
  if (!in(p.idle_timeout, kMinIdleTimeout, kMaxIdleTimeout))
    return "idle timeout must be between 30 and 3600 seconds";
  if (!in(p.retry_time, kMinRetryTime, kMaxRetryTime))
    return "retry time must be between 0.05 and 5 seconds";
  if (!in(p.min_retry_time, kMinRetryTime, p.retry_time))
    return "minimum retry time must be between 0.05 seconds and the retry time";
  return nullptr;
}

// Parses a plain decimal number of seconds ("2", "0.5"). Rejects empty strings,
// signs, exponents, hex, inf/nan and trailing characters.
inline std::optional<float> ParseSeconds(const char *text) {
  if (text == nullptr || !((*text >= '0' && *text <= '9') || *text == '.'))
    return std::nullopt;
  for (const char *c = text; *c; ++c)
    if (!((*c >= '0' && *c <= '9') || *c == '.'))
      return std::nullopt;
  char *end = nullptr;
  errno = 0;
  const double value = std::strtod(text, &end);
  if (errno != 0 || end == text || *end != '\0' || !std::isfinite(value))
    return std::nullopt;
  return static_cast<float>(value);
}

// Builds a policy from command-line style overrides. `lookup(name, &value)`
// returns whether the option is present and sets value to its argument
// (nullptr when the argument is missing). `warn(message, option, value)`
// reports a rejected override. Precedence: a valid command-line value replaces
// the default. A malformed value is ignored. If the combined values are invalid
// (for example a minimum above the base retry time), every default is kept.
template <typename Lookup, typename Warn> nw_timing_policy PolicyFromOptions(Lookup lookup, Warn warn) {
  nw_timing_policy policy;
  struct {
    const char *name;
    float nw_timing_policy::*field;
  } const options[] = {
      {"-netconnecttimeout", &nw_timing_policy::connect_timeout},
      {"-netidletimeout", &nw_timing_policy::idle_timeout},
      {"-netretrytime", &nw_timing_policy::retry_time},
      {"-netminretrytime", &nw_timing_policy::min_retry_time},
  };
  bool any = false;
  for (const auto &option : options) {
    const char *present = nullptr;
    if (!lookup(option.name, &present))
      continue;
    if (auto seconds = ParseSeconds(present)) {
      policy.*option.field = *seconds;
      any = true;
    } else {
      warn("ignoring malformed value", option.name, present ? present : "");
    }
  }
  if (any) {
    if (const char *reason = Validate(policy)) {
      warn(reason, "network timing options", "defaults kept");
      return nw_timing_policy{};
    }
  }
  return policy;
}

} // namespace nw_timing
