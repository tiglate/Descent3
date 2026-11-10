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

#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>

#include "ddio.h"

TEST(D3, DDIO_GetTmpFileName) {
  std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
  std::filesystem::path result = ddio_GetTmpFileName(temp_dir, "prefix_");
  EXPECT_FALSE(result.empty());
  EXPECT_EQ(result.extension(), ".tmp");
  EXPECT_TRUE(canonical(result.parent_path()) == canonical(temp_dir));
}

// we intentionally do NOT use the same RNG as the "game" logic. that RNG is seed-controlled for tests,
// and using it here would introduce an uncontrollable source of randomness from potential tmpdir collisions
TEST(D3, DDIO_GetTmpFileNameDoesNotUseRand) {
  std::filesystem::path temp_dir = std::filesystem::temp_directory_path();
  srand(1234);
  int expected = rand();
  srand(1234);
  ddio_GetTmpFileName(temp_dir, "prefix_");
  EXPECT_EQ(rand(), expected);
}

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include "ddio_lnx.h"

// lnxmouse.cpp reconciles relative mode against the renderer's window; the
// tests run without one.
SDL_Window *GSDLWindow = nullptr;

namespace {
void SendMouseMotion(float xrel, float yrel) {
  SDL_Event event{};
  event.type = SDL_EVENT_MOUSE_MOTION;
  event.motion.xrel = xrel;
  event.motion.yrel = yrel;
  sdlMouseMotionEvent(&event);
}
} // namespace

// In-game dialogs run in a high-resolution coordinate space; the cursor speed
// multiplier keeps them as responsive as the 640x480 menus without changing
// the raw deltas the flight controls read.
TEST(D3, DDIO_MouseCursorSpeedScalesCursorButNotDeltas) {
  ddio_MouseSetLimits(0, 0, 4000, 4000);
  int x0, y0;
  ddio_MouseGetState(&x0, &y0, nullptr, nullptr);

  ddio_MouseSetCursorSpeed(3.0f);
  SendMouseMotion(10.0f, 5.0f);
  int x, y, dx, dy;
  ddio_MouseGetState(&x, &y, &dx, &dy);
  EXPECT_EQ(x - x0, 30);
  EXPECT_EQ(y - y0, 15);
  EXPECT_EQ(dx, 10);
  EXPECT_EQ(dy, 5);

  ddio_MouseSetCursorSpeed(1.0f);
  SendMouseMotion(10.0f, 5.0f);
  ddio_MouseGetState(&x, &y, &dx, &dy);
  EXPECT_EQ(x - x0, 40);
  EXPECT_EQ(y - y0, 20);
}

TEST(D3, DDIO_MouseCursorSpeedRejectsInvalidScale) {
  ddio_MouseSetLimits(0, 0, 4000, 4000);
  int x0, y0;
  ddio_MouseGetState(&x0, &y0, nullptr, nullptr);
  ddio_MouseSetCursorSpeed(0.0f);
  SendMouseMotion(7.0f, 0.0f);
  int x;
  ddio_MouseGetState(&x, nullptr, nullptr, nullptr);
  EXPECT_EQ(x - x0, 7);
}
