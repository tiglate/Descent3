/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sound_buffer_option.h"

#include <gtest/gtest.h>

using sndlib::kMaxSoundBufferFrames;
using sndlib::kMinSoundBufferFrames;
using sndlib::ParseSoundBufferFrames;

TEST(SoundBufferOption, AcceptsValuesInRange) {
  EXPECT_EQ(ParseSoundBufferFrames("1024"), 1024);
  EXPECT_EQ(ParseSoundBufferFrames("64"), kMinSoundBufferFrames);
  EXPECT_EQ(ParseSoundBufferFrames("8192"), kMaxSoundBufferFrames);
  EXPECT_EQ(ParseSoundBufferFrames("0512"), 512);
}

TEST(SoundBufferOption, RejectsMissingAndMalformedValues) {
  for (const char *text : {"", "abc", "512ms", "1e3", " 512", "+512", "-512", "512 ", "12.5"})
    EXPECT_FALSE(ParseSoundBufferFrames(text).has_value()) << '"' << text << '"';
  EXPECT_FALSE(ParseSoundBufferFrames(nullptr).has_value());
}

TEST(SoundBufferOption, RejectsOutOfRangeValues) {
  for (const char *text : {"0", "63", "8193", "2147483648", "99999999999999999999"})
    EXPECT_FALSE(ParseSoundBufferFrames(text).has_value()) << text;
}
