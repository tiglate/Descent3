/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "httpclient.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace {

// Loopback HTTP fixture: /fast answers immediately, /slow waits before the
// response, /stream sends a large body in delayed chunks.
class LocalServer {
public:
  LocalServer() {
    server_.Get("/fast", [](const httplib::Request &, httplib::Response &res) { res.set_content("ok", "text/plain"); });
    server_.Get("/slow", [this](const httplib::Request &, httplib::Response &res) {
      for (int i = 0; i < 40 && !stop_; ++i)
        std::this_thread::sleep_for(50ms);
      res.set_content("late", "text/plain");
    });
    server_.Get("/stream", [this](const httplib::Request &, httplib::Response &res) {
      res.set_content_provider(1 << 20, "application/octet-stream",
                               [this](size_t offset, size_t, httplib::DataSink &sink) {
                                 std::string chunk(4096, 'x');
                                 std::this_thread::sleep_for(10ms);
                                 return !stop_ && sink.write(chunk.data(), chunk.size());
                               });
    });
    port_ = server_.bind_to_any_port("127.0.0.1");
    thread_ = std::thread([this] { server_.listen_after_bind(); });
    server_.wait_until_ready();
  }
  ~LocalServer() {
    stop_ = true;
    server_.stop();
    thread_.join();
  }
  std::string Url() const { return "http://127.0.0.1:" + std::to_string(port_); }

private:
  httplib::Server server_;
  std::thread thread_;
  std::atomic<bool> stop_{false};
  int port_ = 0;
};

} // namespace

TEST(HttpTimeouts, DefaultsMatchPreviousBehavior) {
  D3::HttpTimeouts t;
  EXPECT_EQ(t.connect, 5s);
  EXPECT_EQ(t.read, std::chrono::seconds(CPPHTTPLIB_CLIENT_READ_TIMEOUT_SECOND));
  EXPECT_EQ(t.write, std::chrono::seconds(CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_SECOND));
  EXPECT_EQ(t.Validate(), nullptr);
}

TEST(HttpTimeouts, ParsesWholeSecondsWithinRange) {
  EXPECT_EQ(D3::ParseHttpTimeoutSeconds("1", 60), 1s);
  EXPECT_EQ(D3::ParseHttpTimeoutSeconds("60", 60), 60s);
  EXPECT_EQ(D3::ParseHttpTimeoutSeconds("600", 600), 600s);
  for (const char *bad : {"", "0", "61", "-5", "+5", "1.5", "5s", " 5", "1e2", "99999999999999999999999"})
    EXPECT_FALSE(D3::ParseHttpTimeoutSeconds(bad, 60).has_value()) << '"' << bad << '"';
  EXPECT_FALSE(D3::ParseHttpTimeoutSeconds(nullptr, 60).has_value());
}

TEST(HttpTimeouts, ValidateRejectsOutOfRange) {
  D3::HttpTimeouts t;
  t.connect = 0s;
  EXPECT_NE(t.Validate(), nullptr);
  t = {};
  t.connect = 61s;
  EXPECT_NE(t.Validate(), nullptr);
  t = {};
  t.read = 601s;
  EXPECT_NE(t.Validate(), nullptr);
  t = {};
  t.write = 0s;
  EXPECT_NE(t.Validate(), nullptr);
}

TEST(HttpClientTimeouts, FastRequestSucceeds) {
  LocalServer server;
  D3::HttpClient client(server.Url());
  auto result = client.Get("/fast");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->status, 200);
  EXPECT_EQ(result->body, "ok");
}

TEST(HttpClientTimeouts, ReadTimeoutBoundsADelayedResponse) {
  LocalServer server;
  D3::HttpTimeouts t;
  t.read = 1s;
  D3::HttpClient client(server.Url(), t);
  const auto start = std::chrono::steady_clock::now();
  auto result = client.Get("/slow");
  const auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), httplib::Error::Read);
  EXPECT_GE(elapsed, 900ms);
  EXPECT_LT(elapsed, 1900ms);
}

TEST(HttpClientTimeouts, LongerReadTimeoutWaitsForDelayedResponse) {
  LocalServer server;
  D3::HttpTimeouts t;
  t.read = 5s;
  D3::HttpClient client(server.Url(), t);
  auto result = client.Get("/slow");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->body, "late");
}

TEST(HttpClientTimeouts, InvalidTimeoutsFallBackToBoundedDefaults) {
  LocalServer server;
  D3::HttpTimeouts t;
  t.read = 0s; // invalid: replaced with the default, which outlasts /slow
  D3::HttpClient client(server.Url(), t);
  auto result = client.Get("/slow");
  ASSERT_TRUE(result);
  EXPECT_EQ(result->body, "late");
}

TEST(HttpClientTimeouts, ProgressCallbackCancelsDownload) {
  LocalServer server;
  D3::HttpClient client(server.Url());
  size_t received = 0;
  auto result = client.Get(
      "/stream",
      [&](const char *, size_t len) {
        received += len;
        return true;
      },
      [&](uint64_t current, uint64_t) { return current < 32 * 1024; });
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), httplib::Error::Canceled);
  EXPECT_LT(received, size_t{1} << 20);
}

TEST(HttpClientTimeouts, RefusedConnectionReportsErrorWithoutResponse) {
  int port;
  {
    LocalServer server; // obtain a free port, then close it
    port = std::stoi(server.Url().substr(server.Url().rfind(':') + 1));
  }
  D3::HttpTimeouts t;
  t.connect = 1s;
  D3::HttpClient client("http://127.0.0.1:" + std::to_string(port), t);
  auto result = client.Get("/fast");
  EXPECT_FALSE(result);
  EXPECT_EQ(result.error(), httplib::Error::Connection);
}
