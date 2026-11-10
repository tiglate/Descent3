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

#include "httpclient.h"

#include <cerrno>
#include <cstdlib>

namespace D3 {

const char *HttpTimeouts::Validate() const {
  auto in = [](std::chrono::seconds value, int max) { return value.count() >= kMinSeconds && value.count() <= max; };
  if (!in(connect, kMaxConnectSeconds))
    return "HTTP connect timeout must be between 1 and 60 seconds";
  if (!in(read, kMaxReadWriteSeconds) || !in(write, kMaxReadWriteSeconds))
    return "HTTP read/write timeouts must be between 1 and 600 seconds";
  return nullptr;
}

std::optional<std::chrono::seconds> ParseHttpTimeoutSeconds(const char *text, int max_seconds) {
  if (text == nullptr || *text == '\0')
    return std::nullopt;
  for (const char *c = text; *c; ++c)
    if (*c < '0' || *c > '9')
      return std::nullopt;
  errno = 0;
  const long value = std::strtol(text, nullptr, 10);
  if (errno != 0 || value < HttpTimeouts::kMinSeconds || value > max_seconds)
    return std::nullopt;
  return std::chrono::seconds(value);
}

HttpClient::HttpClient(const std::string &URL) : HttpClient(URL, HttpTimeouts{}) {}

HttpClient::HttpClient(const std::string &URL, const HttpTimeouts &timeouts) {
  const HttpTimeouts &applied = timeouts.Validate() ? HttpTimeouts{} : timeouts;
  m_client = std::make_unique<httplib::Client>(URL);
  m_client->set_follow_location(true); // Follow redirects
  m_client->set_connection_timeout(applied.connect);
  m_client->set_read_timeout(applied.read);
  m_client->set_write_timeout(applied.write);
}

httplib::Result HttpClient::Get(const std::string &URIPath) { return m_client->Get(URIPath); }

httplib::Result HttpClient::Get(const std::string &URIPath, const D3::HttpClient::Progress &progress) {
  return m_client->Get(URIPath, progress);
}

httplib::Result HttpClient::Get(const std::string &URIPath, const httplib::ContentReceiver &content_receiver) {
  return m_client->Get(URIPath, content_receiver);
}

httplib::Result HttpClient::Get(const std::string &URIPath, const httplib::ContentReceiver &content_receiver,
                                const D3::HttpClient::Progress &progress) {
  return m_client->Get(URIPath, content_receiver, progress);
}

void HttpClient::SetProxy(const std::string &proxy_host, uint16_t port) { m_client->set_proxy(proxy_host, port); }

} // namespace D3
