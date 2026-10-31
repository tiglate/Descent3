/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "async_dns.h"

#include <exception>
#include <memory>
#include <utility>

namespace D3 {

AsyncDnsLookup::AsyncDnsLookup(Resolver resolver, ThreadFactory create_thread)
    : resolver_(std::move(resolver)), create_thread_(create_thread) {}

AsyncDnsLookup::~AsyncDnsLookup() { Cancel(); }

bool AsyncDnsLookup::Start(const char *hostname) {
  Cancel();
  if (!hostname || !*hostname)
    return false;

  try {
    hostname_ = hostname;
  } catch (...) {
    return false;
  }
  state_.store(State::Pending, std::memory_order_relaxed);
  thread_ = create_thread_(Worker, this);
  if (!thread_) {
    state_.store(State::Idle, std::memory_order_relaxed);
    hostname_.clear();
    return false;
  }
  return true;
}

int AsyncDnsLookup::Read(uint32_t *ip) {
  if (!ip)
    return -1;

  const State state = state_.load(std::memory_order_acquire);
  if (state == State::Pending)
    return 0;
  if (state == State::Idle)
    return -1;

  // Acquire above publishes ip_. Join before clearing/reusing worker storage,
  // even if the worker has published a result but has not yet returned.
  const bool success = state == State::Ready;
  if (success)
    *ip = ip_;
  Cancel();
  return success ? 1 : -1;
}

void AsyncDnsLookup::Cancel() {
  if (thread_) {
    SDL_WaitThread(thread_, nullptr);
    thread_ = nullptr;
  }
  state_.store(State::Idle, std::memory_order_relaxed);
  hostname_.clear();
}

SDL_Thread *AsyncDnsLookup::CreateThread(SDL_ThreadFunction worker, void *data) {
  return SDL_CreateThread(worker, "dnsworker", data);
}

int SDLCALL AsyncDnsLookup::Worker(void *data) {
  auto &lookup = *static_cast<AsyncDnsLookup *>(data);
  uint32_t ip = 0;
  bool success = false;
  // Do not allow exceptions to cross the SDL thread entry point.
  try {
    success = lookup.resolver_(lookup.hostname_.c_str(), ip);
  } catch (...) {
    success = false;
  }
  lookup.ip_ = ip;
  lookup.state_.store(success ? State::Ready : State::Failed, std::memory_order_release);
  return 0;
}

bool AsyncDnsLookup::ResolveHostname(const char *hostname, uint32_t &ip) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  addrinfo *addresses = nullptr;
  if (getaddrinfo(hostname, nullptr, &hints, &addresses) != 0)
    return false;

  std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> owner(addresses, freeaddrinfo);
  for (auto *address = addresses; address; address = address->ai_next) {
    if (address->ai_family == AF_INET && address->ai_addr && address->ai_addrlen >= sizeof(sockaddr_in)) {
      // Keep the network-byte-order IPv4 value expected by existing callers.
      ip = reinterpret_cast<const sockaddr_in *>(address->ai_addr)->sin_addr.s_addr;
      return true;
    }
  }
  return false;
}

} // namespace D3
