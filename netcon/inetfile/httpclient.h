/*
 * Descent 3
 * Copyright (C) 2024 Descent Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <chrono>
#include <istream>
#include <memory>
#include <optional>
#include <string>

// Include httplib.h before windows.h
#include <httplib.h>
#ifdef WIN32
#include <windows.h>
#endif

namespace D3 {

/**
 * Socket timeouts for HttpClient, in whole seconds. connect bounds establishing
 * the TCP connection; read and write bound each individual socket operation
 * (inactivity), not the total transfer time. Defaults match previous behavior.
 */
struct HttpTimeouts {
  static constexpr int kMinSeconds = 1;
  static constexpr int kMaxConnectSeconds = 60;
  static constexpr int kMaxReadWriteSeconds = 600;

  std::chrono::seconds connect{5};
  std::chrono::seconds read{CPPHTTPLIB_CLIENT_READ_TIMEOUT_SECOND};
  std::chrono::seconds write{CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_SECOND};

  /// @return nullptr when every timeout is within range, otherwise a reason.
  const char *Validate() const;
};

/**
 * Parses a timeout option value: plain decimal digits only, within
 * [HttpTimeouts::kMinSeconds, max_seconds]. Returns no value otherwise.
 */
std::optional<std::chrono::seconds> ParseHttpTimeoutSeconds(const char *text, int max_seconds);

class HttpClient {
public:
  using Progress = httplib::DownloadProgress;
  // no way to test with macro; would have to do a cmake-level compile check
  // #if httplib < 0.23
  // using Progress = httplib::Progress;
  // #endif

  /**
   * Constructor for HttpClient
   * @param URL request URL, should be in form of http://example.com. Don't add "/" on the end as it's part of URIPath.
   * Port can be specified with ":" (http://example.com:8080)
   */
  explicit HttpClient(const std::string &URL);

  /**
   * As above, with explicit timeouts. Invalid timeouts are replaced by the
   * defaults so a request can never wait without bound.
   */
  HttpClient(const std::string &URL, const HttpTimeouts &timeouts);

  httplib::Result Get(const std::string &URIPath);

  /**
   * Get data from URIPath
   * @param URIPath requested path (for example, "/some_dir/my_file.txt")
   * @return standard HTTP code. 200 means is OK.
   */
  httplib::Result Get(const std::string &URIPath, const Progress &progress);

  httplib::Result Get(const std::string &URIPath, const httplib::ContentReceiver &content_receiver);

  httplib::Result Get(const std::string &URIPath, const httplib::ContentReceiver &content_receiver,
                      const Progress &progress);

  void SetProxy(const std::string &proxy_host, uint16_t port);

private:
  std::unique_ptr<httplib::Client> m_client;
};

} // namespace D3
