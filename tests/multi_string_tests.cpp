/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// multi_external.h relies on vecmat types being declared first.
#include "vecmat.h"
#include "multi_external.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

TEST(MultiString, RoundTripsAndKeepsFollowingFieldsAligned) {
  uint8_t data[600] = {};
  int count = 0;
  MultiAddString("hello", data, &count);
  MultiAddByte(42, data, &count);
  EXPECT_EQ(count, 1 + 6 + 1);

  int read = 0;
  char out[32];
  MultiGetString(out, data, &read);
  EXPECT_STREQ(out, "hello");
  EXPECT_EQ(MultiGetByte(data, &read), 42);
}

TEST(MultiString, OversizedNetworkStringIsTruncatedAndTerminated) {
  // A peer sends a 200-byte, unterminated string into a 32-byte field.
  uint8_t data[600];
  std::memset(data, 'A', sizeof(data));
  data[0] = 200;
  data[201] = 7; // field after the string
  char name[32];
  std::memset(name, 'Z', sizeof(name));
  char guard[16];
  std::memset(guard, 'G', sizeof(guard));

  int read = 0;
  MultiGetString(name, data, &read);
  EXPECT_EQ(std::strlen(name), sizeof(name) - 1);
  EXPECT_EQ(read, 201); // advanced past the whole encoded string
  EXPECT_EQ(MultiGetByte(data, &read), 7);
  for (char c : guard)
    EXPECT_EQ(c, 'G');
}

TEST(MultiString, EmptyAndExactFitStrings) {
  uint8_t data[600] = {};
  int count = 0;
  MultiAddString("", data, &count);
  MultiAddString(std::string(31, 'x').c_str(), data, &count);

  int read = 0;
  char a[4], b[32];
  MultiGetString(a, data, &read);
  MultiGetString(b, data, &read);
  EXPECT_STREQ(a, "");
  EXPECT_EQ(std::string(b), std::string(31, 'x'));
  EXPECT_EQ(read, count);
}

TEST(MultiString, LongOutgoingStringIsClampedToTheLengthByte) {
  uint8_t data[600] = {};
  int count = 0;
  std::string longer(300, 'y');
#ifdef NDEBUG
  MultiAddString(longer.c_str(), data, &count);
  EXPECT_EQ(data[0], 0xFF);
  EXPECT_EQ(count, 1 + 0xFF);
  EXPECT_EQ(data[0xFF], '\0');
#else
  GTEST_SKIP() << "assert() fires on over-long strings in debug builds";
#endif
}
