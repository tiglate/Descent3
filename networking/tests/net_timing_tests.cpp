/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "networking.h"
#include "net_timing.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace {

struct Options {
  std::map<std::string, const char *> values;
  std::vector<std::string> warnings;

  nw_timing_policy Build() {
    return nw_timing::PolicyFromOptions(
        [this](const char *name, const char **value) {
          auto it = values.find(name);
          if (it == values.end())
            return false;
          *value = it->second;
          return true;
        },
        [this](const char *reason, const char *option, const char *value) {
          warnings.push_back(std::string(reason) + "|" + option + "|" + value);
        });
  }
};

void ExpectDefaults(const nw_timing_policy &p) {
  EXPECT_FLOAT_EQ(p.connect_timeout, NETTIMEOUT);
  EXPECT_FLOAT_EQ(p.idle_timeout, NETTIMEOUT);
  EXPECT_FLOAT_EQ(p.retry_time, NETRETRYTIME);
  EXPECT_FLOAT_EQ(p.min_retry_time, MIN_NET_RETRYTIME);
}

} // namespace

TEST(NetTiming, DefaultsMatchLegacyConstants) {
  ExpectDefaults(nw_timing_policy{});
  EXPECT_EQ(nw_timing::Validate(nw_timing_policy{}), nullptr);
  Options none;
  ExpectDefaults(none.Build());
  EXPECT_TRUE(none.warnings.empty());
}

TEST(NetTiming, ValidOverridesReplaceDefaults) {
  Options o;
  o.values = {
      {"-netconnecttimeout", "15"}, {"-netidletimeout", "45.5"}, {"-netretrytime", "0.5"}, {"-netminretrytime", ".1"}};
  const auto p = o.Build();
  EXPECT_FLOAT_EQ(p.connect_timeout, 15.0f);
  EXPECT_FLOAT_EQ(p.idle_timeout, 45.5f);
  EXPECT_FLOAT_EQ(p.retry_time, 0.5f);
  EXPECT_FLOAT_EQ(p.min_retry_time, 0.1f);
  EXPECT_TRUE(o.warnings.empty());
}

TEST(NetTiming, ConnectAndIdleAreIndependent) {
  Options o;
  o.values = {{"-netconnecttimeout", "10"}};
  const auto p = o.Build();
  EXPECT_FLOAT_EQ(p.connect_timeout, 10.0f);
  EXPECT_FLOAT_EQ(p.idle_timeout, NETTIMEOUT);
}

TEST(NetTiming, MalformedValuesAreIgnoredWithWarning) {
  for (const char *bad : {"", "abc", "-5", "+5", "1e3", "nan", "inf", "0x10", "5s", " 5"}) {
    Options o;
    o.values = {{"-netconnecttimeout", bad}, {"-netidletimeout", "60"}};
    const auto p = o.Build();
    EXPECT_FLOAT_EQ(p.connect_timeout, NETTIMEOUT) << bad;
    EXPECT_FLOAT_EQ(p.idle_timeout, 60.0f) << bad;
    ASSERT_EQ(o.warnings.size(), 1u) << bad;
  }
  Options missing;
  missing.values = {{"-netretrytime", nullptr}};
  ExpectDefaults(missing.Build());
  EXPECT_EQ(missing.warnings.size(), 1u);
}

TEST(NetTiming, OutOfRangeOrInconsistentValuesKeepAllDefaults) {
  const std::vector<std::map<std::string, const char *>> cases = {
      {{"-netconnecttimeout", "0"}},
      {{"-netconnecttimeout", "601"}},
      {{"-netidletimeout", "29"}},
      {{"-netidletimeout", "3601"}},
      {{"-netretrytime", "0.01"}},
      {{"-netretrytime", "6"}},
      {{"-netminretrytime", "0.9"}}, // above the default 0.75 retry time
      {{"-netretrytime", "0.3"}, {"-netminretrytime", "0.4"}, {"-netconnecttimeout", "20"}},
      {{"-netconnecttimeout", "99999999999999999999999999999999999999999"}},
  };
  for (const auto &values : cases) {
    Options o;
    o.values = values;
    ExpectDefaults(o.Build());
    EXPECT_EQ(o.warnings.size(), 1u);
  }
}

TEST(NetTiming, ValidateRejectsNonFiniteAndBoundaryValues) {
  nw_timing_policy p;
  p.idle_timeout = std::numeric_limits<float>::quiet_NaN();
  EXPECT_NE(nw_timing::Validate(p), nullptr);
  p = {};
  p.retry_time = std::numeric_limits<float>::infinity();
  EXPECT_NE(nw_timing::Validate(p), nullptr);
  p = {};
  p.connect_timeout = nw_timing::kMinConnectTimeout;
  p.idle_timeout = nw_timing::kMinIdleTimeout;
  p.retry_time = p.min_retry_time = nw_timing::kMinRetryTime;
  EXPECT_EQ(nw_timing::Validate(p), nullptr);
  p.connect_timeout = nw_timing::kMaxConnectTimeout;
  p.idle_timeout = nw_timing::kMaxIdleTimeout;
  p.retry_time = p.min_retry_time = nw_timing::kMaxRetryTime;
  EXPECT_EQ(nw_timing::Validate(p), nullptr);
}
