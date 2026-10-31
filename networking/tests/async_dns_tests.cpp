/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "async_dns.h"

#include <SDL3/SDL_timer.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using D3::AsyncDnsLookup;
using namespace std::chrono_literals;

class ResolverGate {
public:
  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    entered_ = true;
    changed_.notify_all();
    // Bound failure time so a broken test never leaves a worker hung forever.
    changed_.wait_for(lock, 5s, [this] { return released_; });
  }

  bool WaitForEntry() {
    std::unique_lock<std::mutex> lock(mutex_);
    return changed_.wait_for(lock, 2s, [this] { return entered_; });
  }

  void Release() {
    std::lock_guard<std::mutex> lock(mutex_);
    released_ = true;
    changed_.notify_all();
  }

private:
  std::mutex mutex_;
  std::condition_variable changed_;
  bool entered_ = false;
  bool released_ = false;
};

int ReadUntilFinished(AsyncDnsLookup &lookup, uint32_t &ip) {
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  int result;
  do {
    result = lookup.Read(&ip);
    if (result != 0)
      return result;
    SDL_Delay(1);
  } while (std::chrono::steady_clock::now() < deadline);
  return result;
}

class AsyncDnsLookupTest : public testing::Test {
protected:
  void SetUp() override {
#ifdef _WIN32
    WSADATA data;
    ASSERT_EQ(WSAStartup(MAKEWORD(2, 2), &data), 0);
#endif
  }

  void TearDown() override {
#ifdef _WIN32
    WSACleanup();
#endif
  }
};

TEST_F(AsyncDnsLookupTest, OwnsHostnameAndConsumesPublishedResultOnce) {
  ResolverGate gate;
  std::string resolved_host;
  AsyncDnsLookup lookup([&](const char *host, uint32_t &ip) {
    gate.Wait();
    resolved_host = host;
    ip = 0x12345678;
    return true;
  });
  char hostname[] = "original";
  ASSERT_TRUE(lookup.Start(hostname));
  ASSERT_TRUE(gate.WaitForEntry());
  hostname[0] = 'X';
  uint32_t ip = 0;
  EXPECT_EQ(lookup.Read(&ip), 0);
  EXPECT_EQ(ip, 0u);
  gate.Release();
  ASSERT_EQ(ReadUntilFinished(lookup, ip), 1);
  EXPECT_EQ(ip, 0x12345678u);
  EXPECT_EQ(resolved_host, "original");
  EXPECT_EQ(lookup.Read(&ip), -1);
}

TEST_F(AsyncDnsLookupTest, ReplacesUnconsumedRequestWithoutFreeingStaticStorage) {
  ResolverGate gate;
  std::atomic<bool> first_finished{false};
  AsyncDnsLookup lookup([&](const char *host, uint32_t &ip) {
    if (std::string(host) == "first") {
      gate.Wait();
      ip = 1;
      first_finished = true;
    } else {
      ip = 2;
    }
    return true;
  });
  ASSERT_TRUE(lookup.Start("first"));
  ASSERT_TRUE(gate.WaitForEntry());
  uint32_t ip = 0;
  EXPECT_EQ(lookup.Read(&ip), 0);
  std::thread release([&] { gate.Release(); });
  const bool started = lookup.Start("second");
  release.join();
  ASSERT_TRUE(started);
  EXPECT_TRUE(first_finished);
  ASSERT_EQ(ReadUntilFinished(lookup, ip), 1);
  EXPECT_EQ(ip, 2u);
}

TEST_F(AsyncDnsLookupTest, CancelJoinsPendingWorkerAndDiscardsResult) {
  ResolverGate gate;
  std::atomic<bool> finished{false};
  AsyncDnsLookup lookup([&](const char *, uint32_t &ip) {
    gate.Wait();
    ip = 1;
    finished = true;
    return true;
  });
  ASSERT_TRUE(lookup.Start("pending"));
  ASSERT_TRUE(gate.WaitForEntry());
  std::thread release([&] { gate.Release(); });
  lookup.Cancel();
  release.join();
  EXPECT_TRUE(finished);
  uint32_t ip = 99;
  EXPECT_EQ(lookup.Read(&ip), -1);
  EXPECT_EQ(ip, 99u);
  lookup.Cancel();
}

TEST_F(AsyncDnsLookupTest, DestructorJoinsPendingWorker) {
  ResolverGate gate;
  std::atomic<bool> finished{false};
  std::thread release;
  {
    AsyncDnsLookup lookup([&](const char *, uint32_t &) {
      gate.Wait();
      finished = true;
      return false;
    });
    ASSERT_TRUE(lookup.Start("pending"));
    ASSERT_TRUE(gate.WaitForEntry());
    release = std::thread([&] { gate.Release(); });
  }
  release.join();
  EXPECT_TRUE(finished);
}

TEST_F(AsyncDnsLookupTest, FailureIsConsumedAndLookupCanBeRetried) {
  AsyncDnsLookup lookup([](const char *host, uint32_t &ip) {
    ip = 7;
    return std::string(host) == "success";
  });
  uint32_t ip = 99;
  ASSERT_TRUE(lookup.Start("failure"));
  EXPECT_EQ(ReadUntilFinished(lookup, ip), -1);
  EXPECT_EQ(ip, 99u);
  EXPECT_EQ(lookup.Read(&ip), -1);
  ASSERT_TRUE(lookup.Start("success"));
  EXPECT_EQ(ReadUntilFinished(lookup, ip), 1);
  EXPECT_EQ(ip, 7u);
}

TEST_F(AsyncDnsLookupTest, ThreadCreationFailureDoesNotLeavePendingRequest) {
  static bool fail_next;
  fail_next = true;
  auto factory = [](SDL_ThreadFunction worker, void *data) -> SDL_Thread * {
    if (fail_next) {
      fail_next = false;
      return nullptr;
    }
    return SDL_CreateThread(worker, "test-dns", data);
  };
  AsyncDnsLookup lookup(
      [](const char *, uint32_t &ip) {
        ip = 3;
        return true;
      },
      factory);
  uint32_t ip = 99;
  EXPECT_FALSE(lookup.Start("failure"));
  EXPECT_EQ(lookup.Read(&ip), -1);
  EXPECT_EQ(ip, 99u);
  ASSERT_TRUE(lookup.Start("retry"));
  EXPECT_EQ(ReadUntilFinished(lookup, ip), 1);
  EXPECT_EQ(ip, 3u);
}

TEST_F(AsyncDnsLookupTest, InvalidInputsDoNotDereferenceNullOrStartLookup) {
  AsyncDnsLookup lookup([](const char *, uint32_t &ip) {
    ip = 4;
    return true;
  });
  uint32_t ip = 99;
  EXPECT_EQ(lookup.Read(&ip), -1);
  EXPECT_FALSE(lookup.Start(nullptr));
  EXPECT_FALSE(lookup.Start(""));
  ASSERT_TRUE(lookup.Start("valid"));
  EXPECT_EQ(lookup.Read(nullptr), -1);
  EXPECT_EQ(ReadUntilFinished(lookup, ip), 1);
  EXPECT_EQ(ip, 4u);
}

TEST_F(AsyncDnsLookupTest, ResolverExceptionBecomesFailure) {
  AsyncDnsLookup lookup([](const char *, uint32_t &) -> bool { throw std::runtime_error("resolver failed"); });
  ASSERT_TRUE(lookup.Start("failure"));
  uint32_t ip = 99;
  EXPECT_EQ(ReadUntilFinished(lookup, ip), -1);
  EXPECT_EQ(ip, 99u);
}

TEST_F(AsyncDnsLookupTest, ResolvesNumericIpv4InNetworkByteOrder) {
  AsyncDnsLookup lookup;
  ASSERT_TRUE(lookup.Start("127.0.0.1"));
  uint32_t ip = 0;
  ASSERT_EQ(ReadUntilFinished(lookup, ip), 1);
  const auto *bytes = reinterpret_cast<const unsigned char *>(&ip);
  EXPECT_EQ(bytes[0], 127);
  EXPECT_EQ(bytes[1], 0);
  EXPECT_EQ(bytes[2], 0);
  EXPECT_EQ(bytes[3], 1);
}

} // namespace
