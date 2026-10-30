/*
 * Descent 3
 * Copyright (C) 2024 Parallax Software
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


///////////////////////////////////////////////////////////////////////
// llsGeometry
#include "ddsndgeometry.h"

// specify a sound library to associate geometry with
bool llsGeometry::Init([[maybe_unused]] llsSystem *snd_sys) { return false; }

// closes low level geometry system.
void llsGeometry::Shutdown() {}

void llsGeometry::StartFrame() {}
void llsGeometry::EndFrame() {}

// clears out geometry info
void llsGeometry::Clear() {}

// polygon lists
// is a group cached?, check before rendering it.
void llsGeometry::IsGroupValid([[maybe_unused]] int group) {}

// marks beginning of a list of polygons to render, (-1 group for non cache)
void llsGeometry::StartPolygonGroup([[maybe_unused]] int group) {}

// ends a list of polygons to render.
void llsGeometry::EndPolygonGroup([[maybe_unused]] int group) {}

// renders a group.
void llsGeometry::RenderGroup([[maybe_unused]] int group) {}

// primatives, nv = number of verts, and verts is an array of pointers to vertices.
// you can pass a sound material value if you want special reflective properties on this polygon.
void llsGeometry::AddPoly([[maybe_unused]] int nv, [[maybe_unused]] vector **verts, [[maybe_unused]] unsigned tag,
                          [[maybe_unused]] tSoundMaterial material) {}

// 4 verts here.
void llsGeometry::AddQuad([[maybe_unused]] unsigned tag, [[maybe_unused]] vector **verts) {}

// 3 verts here.
void llsGeometry::AddTriangle([[maybe_unused]] unsigned tag, [[maybe_unused]] vector **verts) {}

void llsGeometry::CreateMaterial([[maybe_unused]] tSoundMaterial material, [[maybe_unused]] float transmit_gain,
                                 [[maybe_unused]] float transmit_highfreq, [[maybe_unused]] float reflect_gain,
                                 [[maybe_unused]] float reflect_highfreq) {}

void llsGeometry::DestroyMaterial([[maybe_unused]] tSoundMaterial material) {}
