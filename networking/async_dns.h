/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef D3_ASYNC_DNS_H
#define D3_ASYNC_DNS_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include <SDL3/SDL_thread.h>

namespace D3 {

// Start, Read and Cancel are called serially by the game thread. Only the worker
// publishes results; its hostname and storage remain owned until it is joined.
class AsyncDnsLookup {
public:
  using Resolver = std::function<bool(const char *, uint32_t &)>;
  using ThreadFactory = SDL_Thread *(*)(SDL_ThreadFunction, void *);

  explicit AsyncDnsLookup(Resolver resolver = ResolveHostname, ThreadFactory create_thread = CreateThread);
  ~AsyncDnsLookup();
  AsyncDnsLookup(const AsyncDnsLookup &) = delete;
  AsyncDnsLookup &operator=(const AsyncDnsLookup &) = delete;

  bool Start(const char *hostname);
  // 0 = pending, 1 = success (consumes result), -1 = no request/failure.
  int Read(uint32_t *ip);
  // Discards the result and joins the worker. OS resolution itself cannot be
  // interrupted, so cancellation/replacement may wait for the resolver to return.
  void Cancel();

private:
  enum class State { Idle, Pending, Ready, Failed };
  static bool ResolveHostname(const char *hostname, uint32_t &ip);
  static SDL_Thread *CreateThread(SDL_ThreadFunction worker, void *data);
  static int SDLCALL Worker(void *data);

  Resolver resolver_;
  ThreadFactory create_thread_;
  std::string hostname_;
  SDL_Thread *thread_ = nullptr;
  uint32_t ip_ = 0;
  std::atomic<State> state_{State::Idle};
};

} // namespace D3

#endif
