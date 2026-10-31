/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ddio_common.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <deque>
#include <string>

bool con_raw_Create();
void con_raw_Destroy();
void con_raw_Defer();
bool con_raw_Input(char *buf, int buflen);

namespace {
std::deque<int> keys;
int key_polls = 0;

#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
bool track_allocations = false;
int fail_after = -1;
std::array<void *, 64> buffers{};
size_t LiveBuffers() {
  size_t count = 0;
  for (auto *buffer : buffers)
    count += buffer != nullptr;
  return count;
}
#endif
} // namespace

// Supply keyboard events to the production console without opening input devices.
int ddio_KeyInKey() {
  ++key_polls;
  if (keys.empty())
    return 0;
  const int key = keys.front();
  keys.pop_front();
  return key;
}
int ddio_KeyToAscii(int key) { return key; }

#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
extern "C" void *__real_malloc(size_t size);
extern "C" void __real_free(void *pointer);
extern "C" void *__wrap_malloc(size_t size) {
  if (track_allocations && fail_after == 0) {
    fail_after = -1;
    return nullptr;
  }
  if (track_allocations && fail_after > 0)
    --fail_after;
  void *pointer = __real_malloc(size);
  if (track_allocations && pointer) {
    for (auto &buffer : buffers) {
      if (!buffer) {
        buffer = pointer;
        break;
      }
    }
  }
  return pointer;
}
extern "C" void __wrap_free(void *pointer) {
  if (pointer) {
    for (auto &buffer : buffers) {
      if (buffer == pointer) {
        buffer = nullptr;
        break;
      }
    }
  }
  __real_free(pointer);
}
#endif

namespace {
class RawConsoleTest : public testing::Test {
protected:
  void SetUp() override {
    con_raw_Destroy();
    keys.clear();
    key_polls = 0;
  }
  void TearDown() override {
    con_raw_Destroy();
    keys.clear();
#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
    const size_t live = LiveBuffers();
    // Do not keep a leaked original-loader buffer artificially reachable to LSan.
    buffers.fill(nullptr);
    EXPECT_EQ(live, 0u);
#endif
  }
  bool Create(int failure = -1) {
#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
    track_allocations = true;
    fail_after = failure;
#endif
    const bool created = con_raw_Create();
#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
    track_allocations = false;
    fail_after = -1;
#endif
    return created;
  }
  void Enter(const std::string &text) {
    for (unsigned char character : text)
      keys.push_back(character);
    keys.push_back(KEY_ENTER);
    con_raw_Defer();
  }
};

TEST_F(RawConsoleTest, ReadsTypedCommandOnce) {
  ASSERT_TRUE(Create());
  Enter("hi");
  char command[16]{};
  EXPECT_TRUE(con_raw_Input(command, sizeof(command)));
  EXPECT_STREQ(command, "hi");
  EXPECT_FALSE(con_raw_Input(command, sizeof(command)));
}

TEST_F(RawConsoleTest, BackspaceEditingPreservesTheCommand) {
  ASSERT_TRUE(Create());
  keys = {'a', 'b', KEY_BACKSP, 'c', KEY_ENTER};
  con_raw_Defer();
  char command[16]{};
  EXPECT_TRUE(con_raw_Input(command, sizeof(command)));
  EXPECT_STREQ(command, "ac");
}

TEST_F(RawConsoleTest, DestroyIsRepeatableAndClosedConsoleDoesNotPollKeys) {
  ASSERT_TRUE(Create());
  con_raw_Destroy();
  con_raw_Destroy();
  keys = {'x', KEY_ENTER};
  con_raw_Defer();
  EXPECT_EQ(key_polls, 0);
  char command[16] = "old";
  EXPECT_FALSE(con_raw_Input(command, sizeof(command)));
  EXPECT_STREQ(command, "");
}

TEST_F(RawConsoleTest, ReinitializationReplacesBuffersAndClearsPendingInput) {
  ASSERT_TRUE(Create());
  Enter("old");
  ASSERT_TRUE(Create());
#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
  EXPECT_EQ(LiveBuffers(), 2u);
#endif
  char command[16]{};
  EXPECT_FALSE(con_raw_Input(command, sizeof(command)));
  Enter("new");
  EXPECT_TRUE(con_raw_Input(command, sizeof(command)));
  EXPECT_STREQ(command, "new");
}

TEST_F(RawConsoleTest, InvalidOutputStorageDoesNotConsumePendingCommand) {
  ASSERT_TRUE(Create());
  Enter("hi");
  char command[16] = "unchanged";
  EXPECT_FALSE(con_raw_Input(nullptr, 16));
  EXPECT_FALSE(con_raw_Input(command, 0));
  EXPECT_FALSE(con_raw_Input(command, -1));
  EXPECT_STREQ(command, "unchanged");
  EXPECT_TRUE(con_raw_Input(command, sizeof(command)));
  EXPECT_STREQ(command, "hi");
}

#ifdef RAW_CONSOLE_TEST_WRAP_ALLOCATIONS
TEST_F(RawConsoleTest, FirstAllocationFailureReleasesAllBuffers) {
  EXPECT_FALSE(Create(0));
  EXPECT_EQ(LiveBuffers(), 0u);
  con_raw_Destroy();
  ASSERT_TRUE(Create());
  EXPECT_EQ(LiveBuffers(), 2u);
}

TEST_F(RawConsoleTest, SecondAllocationFailureCanBeRetriedWithoutLeaks) {
  EXPECT_FALSE(Create(1));
  EXPECT_EQ(LiveBuffers(), 0u);
  ASSERT_TRUE(Create());
  con_raw_Destroy();
  EXPECT_EQ(LiveBuffers(), 0u);
}

TEST_F(RawConsoleTest, FailedReinitializationPreservesPendingCommand) {
  for (int failure : {0, 1}) {
    ASSERT_TRUE(Create());
    Enter("hi");
    EXPECT_FALSE(Create(failure));
    EXPECT_EQ(LiveBuffers(), 2u);
    char command[16]{};
    EXPECT_TRUE(con_raw_Input(command, sizeof(command)));
    EXPECT_STREQ(command, "hi");
    con_raw_Destroy();
    EXPECT_EQ(LiveBuffers(), 0u);
  }
}

TEST_F(RawConsoleTest, FailedInitializationDoesNotPollKeyboard) {
  ASSERT_FALSE(Create(1));
  keys = {'h', 'i', KEY_ENTER};
  con_raw_Defer();
  EXPECT_EQ(key_polls, 0);
  ASSERT_TRUE(Create());
  con_raw_Defer();
  char command[16]{};
  EXPECT_TRUE(con_raw_Input(command, sizeof(command)));
  EXPECT_STREQ(command, "hi");
}
#endif

} // namespace
