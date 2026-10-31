/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "registry.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#ifdef REGISTRY_TEST_WRAP_MALLOC
namespace {
thread_local int fail_allocation_after = -1;
}
extern "C" void *__real_malloc(size_t size);
extern "C" void *__wrap_malloc(size_t size) {
  if (fail_allocation_after == 0) {
    fail_allocation_after = -1;
    return nullptr;
  }
  if (fail_allocation_after > 0)
    --fail_allocation_after;
  return __real_malloc(size);
}
#endif

namespace {

class RegistryTest : public testing::Test {
protected:
  void SetUp() override {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() / ("descent3-registry-" + std::to_string(stamp));
    registry_ = std::make_unique<CRegistry>(path_.string().c_str());
    registry_->CreateKey("Settings");
  }
  void TearDown() override {
    registry_.reset();
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }
  bool WriteString(const char *name, const char *value) {
    return registry_->CreateRecord(name, REGT_STRING, const_cast<char *>(value));
  }
  std::filesystem::path path_;
  std::unique_ptr<CRegistry> registry_;
};

TEST_F(RegistryTest, CaseInsensitiveReplacementKeepsOneRecord) {
  ASSERT_TRUE(WriteString("Value", "before"));
  ASSERT_TRUE(WriteString("vAlUe", "after"));
  char value[32]{};
  auto *record = registry_->LookupRecord("VALUE", value);
  ASSERT_NE(record, nullptr);
  EXPECT_STREQ(value, "after");
  EXPECT_EQ(record->next, nullptr);
  EXPECT_EQ(registry_->GetDataSize("value"), 6);
}

TEST_F(RegistryTest, ReplacementCanChangeStoredType) {
  ASSERT_TRUE(WriteString("Value", "old"));
  int number = 0x12345678;
  ASSERT_TRUE(registry_->CreateRecord("Value", REGT_DWORD, &number));
  int read = 0;
  ASSERT_NE(registry_->LookupRecord("Value", &read), nullptr);
  EXPECT_EQ(read, number);
  EXPECT_EQ(registry_->GetDataSize("Value"), sizeof(int));
  ASSERT_TRUE(WriteString("Value", "new"));
  char value[32]{};
  ASSERT_NE(registry_->LookupRecord("Value", value), nullptr);
  EXPECT_STREQ(value, "new");
}

TEST_F(RegistryTest, ReplacementInputCanAliasTheOldValue) {
  ASSERT_TRUE(WriteString("Value", "same"));
  char buffer[32]{};
  auto *record = registry_->LookupRecord("Value", buffer);
  ASSERT_NE(record, nullptr);
  ASSERT_TRUE(registry_->CreateRecord("Value", REGT_STRING, record->data));
  ASSERT_NE(registry_->LookupRecord("Value", buffer), nullptr);
  EXPECT_STREQ(buffer, "same");
}

TEST_F(RegistryTest, InvalidInputsCannotInsertOrReplaceARecord) {
  ASSERT_TRUE(WriteString("Value", "original"));
  int number = 42;
  EXPECT_FALSE(registry_->CreateRecord("Value", 99, &number));
  EXPECT_FALSE(registry_->CreateRecord("Unknown", 99, &number));
  EXPECT_FALSE(registry_->CreateRecord(nullptr, REGT_DWORD, &number));
  EXPECT_FALSE(registry_->CreateRecord("Value", REGT_DWORD, nullptr));
  const std::string long_name(MAX_RECORD_NAME, 'x');
  EXPECT_FALSE(registry_->CreateRecord(long_name.c_str(), REGT_DWORD, &number));
  char buffer[32]{};
  ASSERT_NE(registry_->LookupRecord("Value", buffer), nullptr);
  EXPECT_STREQ(buffer, "original");
  EXPECT_EQ(registry_->GetDataSize("Unknown"), 0);
}

TEST_F(RegistryTest, MaximumRecordNameFitsAndRoundTrips) {
  const std::string name(MAX_RECORD_NAME - 1, 'n');
  ASSERT_TRUE(WriteString(name.c_str(), "data"));
  registry_->Export();
  ASSERT_TRUE(registry_->Import());
  ASSERT_TRUE(registry_->LookupKey("settings"));
  char buffer[32]{};
  ASSERT_NE(registry_->LookupRecord(name.c_str(), buffer), nullptr);
  EXPECT_STREQ(buffer, "data");
}

TEST_F(RegistryTest, ExportImportPreservesStringAndDwordValues) {
  ASSERT_TRUE(WriteString("Text", "some value"));
  int number = 0x1234abcd;
  ASSERT_TRUE(registry_->CreateRecord("Number", REGT_DWORD, &number));
  registry_->Export();
  ASSERT_TRUE(registry_->Import());
  ASSERT_TRUE(registry_->LookupKey("Settings"));
  char buffer[64]{};
  int value = 0;
  ASSERT_NE(registry_->LookupRecord("Text", buffer), nullptr);
  ASSERT_NE(registry_->LookupRecord("Number", &value), nullptr);
  EXPECT_STREQ(buffer, "some value");
  EXPECT_EQ(value, number);
}

TEST_F(RegistryTest, EmptyImportClearsThePreviouslySelectedKey) {
  ASSERT_TRUE(WriteString("Value", "old"));
  std::ofstream(path_, std::ios::trunc).close();
  ASSERT_TRUE(registry_->Import());
  EXPECT_FALSE(WriteString("Value", "new"));
  EXPECT_EQ(registry_->GetDataSize("Value"), 0);
  registry_->CreateKey("New");
  EXPECT_TRUE(WriteString("Value", "new"));
}

TEST_F(RegistryTest, InvalidKeyCannotLeaveThePreviousKeySelected) {
  ASSERT_TRUE(WriteString("Value", "old"));
  const std::string long_key(MAX_RECORD_NAME, 'k');
  registry_->CreateKey(long_key.c_str());
  EXPECT_FALSE(WriteString("Value", "new"));
  registry_->CreateKey(nullptr);
  EXPECT_FALSE(WriteString("Value", "new"));
  ASSERT_TRUE(registry_->LookupKey("Settings"));
  char value[32]{};
  ASSERT_NE(registry_->LookupRecord("Value", value), nullptr);
  EXPECT_STREQ(value, "old");
}

#ifdef REGISTRY_TEST_WRAP_MALLOC
class AllocationFailure {
public:
  explicit AllocationFailure(int after) { fail_allocation_after = after; }
  ~AllocationFailure() { fail_allocation_after = -1; }
};

TEST_F(RegistryTest, NewRecordFailureLeavesRegistryUnchanged) {
  for (bool has_record : {false, true}) {
    if (has_record) {
      ASSERT_TRUE(WriteString("Existing", "preserved"));
    }
    for (int allocation : {0, 1}) {
      bool created;
      {
        AllocationFailure fail(allocation);
        created = WriteString("New", "value");
      }
      EXPECT_FALSE(created);
      EXPECT_EQ(registry_->GetDataSize("New"), 0);
      if (has_record) {
        char buffer[32]{};
        auto *record = registry_->LookupRecord("Existing", buffer);
        ASSERT_NE(record, nullptr);
        EXPECT_STREQ(buffer, "preserved");
        EXPECT_EQ(record->next, nullptr);
      }
    }
  }
  EXPECT_TRUE(WriteString("New", "retry"));
}

TEST_F(RegistryTest, NewDwordRecordFailureLeavesNoPartialRecord) {
  int number = 42;
  for (int allocation : {0, 1}) {
    bool created;
    {
      AllocationFailure fail(allocation);
      created = registry_->CreateRecord("Number", REGT_DWORD, &number);
    }
    EXPECT_FALSE(created);
    EXPECT_EQ(registry_->GetDataSize("Number"), 0);
  }
  EXPECT_TRUE(registry_->CreateRecord("Number", REGT_DWORD, &number));
}

TEST_F(RegistryTest, ReplacementAllocationFailurePreservesOldValueAndType) {
  ASSERT_TRUE(WriteString("Value", "original"));
  int number = 42;
  char replacement[] = "new";
  for (char type : {REGT_STRING, REGT_DWORD}) {
    bool updated;
    {
      AllocationFailure fail(0);
      updated =
          registry_->CreateRecord("Value", type, type == REGT_STRING ? static_cast<void *>(replacement) : &number);
    }
    EXPECT_FALSE(updated);
    char buffer[32]{};
    auto *record = registry_->LookupRecord("Value", buffer);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ(record->type, REGT_STRING);
    EXPECT_STREQ(buffer, "original");
    EXPECT_EQ(record->next, nullptr);
  }
  EXPECT_TRUE(WriteString("Value", "retry"));
}

TEST_F(RegistryTest, FailedKeyAllocationCannotWriteIntoPreviousKey) {
  ASSERT_TRUE(WriteString("Value", "old"));
  {
    AllocationFailure fail(0);
    registry_->CreateKey("Other");
  }
  EXPECT_FALSE(WriteString("Value", "new"));
  EXPECT_FALSE(registry_->LookupKey("Other"));
  ASSERT_TRUE(registry_->LookupKey("Settings"));
  char value[32]{};
  ASSERT_NE(registry_->LookupRecord("Value", value), nullptr);
  EXPECT_STREQ(value, "old");
}
#endif

} // namespace
