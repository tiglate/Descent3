/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "inffile.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace {

int LexCommand(const char *command) { return std::strcmp(command, "set") == 0 ? 7 : INFFILE_ERROR; }

class InfFileTest : public testing::Test {
protected:
  void SetUp() override {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    path_ = std::filesystem::temp_directory_path() / ("descent3-inf-" + std::to_string(stamp));
  }
  void TearDown() override {
    inf_.Close();
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }
  bool Open(const char *text) {
    inf_.Close();
    {
      std::ofstream file(path_);
      file << text;
      if (!file.good())
        return false;
    }
    return inf_.Open(path_, nullptr, LexCommand);
  }
  void DefineSymbol() {
    ASSERT_TRUE(inf_.ReadLine());
    char operand[INFFILE_LINELEN];
    ASSERT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_SYMBOL);
  }
  std::filesystem::path path_;
  InfFile inf_;
};

TEST_F(InfFileTest, OversizedSymbolReturnsErrorWithoutWritingPastBuffer) {
  ASSERT_TRUE(Open("#X abcdefghijklmno\nset #X\n"));
  DefineSymbol();
  ASSERT_TRUE(inf_.ReadLine());
  std::array<char, 4> operand{};
  EXPECT_EQ(inf_.ParseLine(operand.data(), operand.size()), INFFILE_ERROR);
  EXPECT_STREQ(operand.data(), "");
}

TEST_F(InfFileTest, SymbolValueExactlyFitsWithTerminator) {
  ASSERT_TRUE(Open("#X abc\nset #X\n"));
  DefineSymbol();
  ASSERT_TRUE(inf_.ReadLine());
  char operand[4]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "abc");
}

TEST_F(InfFileTest, SymbolLookupDoesNotTruncateTheSymbolNameToOutputCapacity) {
  ASSERT_TRUE(Open("#LONGNAME x\nset #LONGNAME\n"));
  DefineSymbol();
  ASSERT_TRUE(inf_.ReadLine());
  char operand[2]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "x");
}

TEST_F(InfFileTest, MissingSymbolReturnsErrorWithEmptyOutput) {
  ASSERT_TRUE(Open("set #MISSING\n"));
  ASSERT_TRUE(inf_.ReadLine());
  char operand[32]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_ERROR);
  EXPECT_STREQ(operand, "");
}

TEST_F(InfFileTest, LiteralTruncationRemainsCompatible) {
  ASSERT_TRUE(Open("set abcdef\n"));
  ASSERT_TRUE(inf_.ReadLine());
  char operand[4]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "abc");
}

TEST_F(InfFileTest, InvalidOutputCapacityCanBeRetriedWithoutConsumingTheLine) {
  ASSERT_TRUE(Open("set value\n"));
  ASSERT_TRUE(inf_.ReadLine());
  char operand[16] = "unchanged";
  EXPECT_EQ(inf_.ParseLine(nullptr, 16), INFFILE_ERROR);
  EXPECT_EQ(inf_.ParseLine(operand, 0), INFFILE_ERROR);
  EXPECT_EQ(inf_.ParseLine(operand, -1), INFFILE_ERROR);
  EXPECT_STREQ(operand, "unchanged");
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "value");
}

TEST_F(InfFileTest, EmptySymbolFitsOneByteOutput) {
  ASSERT_TRUE(Open("#X\nset #X\n"));
  DefineSymbol();
  ASSERT_TRUE(inf_.ReadLine());
  char operand[1] = {'x'};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "");
}

TEST_F(InfFileTest, DelimiterOnlyLineIsEndOfLine) {
  ASSERT_TRUE(Open(" \t=: \n"));
  ASSERT_TRUE(inf_.ReadLine());
  char operand[16]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_EOL);
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_EOL);
}

TEST_F(InfFileTest, ParsesMultipleCommandsAndComments) {
  ASSERT_TRUE(Open("set one;set two\n@ comment\n"));
  ASSERT_TRUE(inf_.ReadLine());
  char operand[16]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "one");
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), 7);
  EXPECT_STREQ(operand, "two");
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_EOL);
  ASSERT_TRUE(inf_.ReadLine());
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_COMMENT);
}

TEST_F(InfFileTest, ClosedParserDoesNotAccessStaleLineStorage) {
  char operand[16]{};
  EXPECT_FALSE(inf_.ReadLine());
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_EOL);
  ASSERT_TRUE(Open("set value\n"));
  ASSERT_TRUE(inf_.ReadLine());
  inf_.Close();
  EXPECT_FALSE(inf_.ReadLine());
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_EOL);
}

TEST_F(InfFileTest, ClosingClearsSymbolsBeforeReopen) {
  ASSERT_TRUE(Open("#X value\n"));
  DefineSymbol();
  ASSERT_TRUE(Open("set #X\n"));
  ASSERT_TRUE(inf_.ReadLine());
  char operand[16]{};
  EXPECT_EQ(inf_.ParseLine(operand, sizeof(operand)), INFFILE_ERROR);
}

} // namespace
