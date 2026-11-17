// Copyright 2025 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

// Canonical controller layout classes, shared by the SDL input layer and the
// runtime. Deliberately free of SDL types so any host component can include it.
//
// Positional binds the GameCube pad to the SDL button *position* (south, east,
// west, north). Nintendo swaps A<->B and X<->Y so the printed labels match.
// D-pad, sticks and triggers are never swapped. The class is resolved once per
// pad at input init; no controller profile is read or written to pick it.
namespace ciface::SDL
{
enum class LayoutClass
{
  Positional,
  Nintendo,
};

namespace detail
{
// Header-only on purpose: the override has to be visible to the input layer and
// to the runtime without adding a link dependency between them.
inline std::atomic<int>& LayoutClassOverride()
{
  static std::atomic<int> value{-1};
  return value;
}
}  // namespace detail

// override_class: -1 = auto (per pad), 0 = force positional, 1 = force nintendo.
inline void SetLayoutClassOverride(int override_class)
{
  detail::LayoutClassOverride().store(override_class);
}

inline int GetLayoutClassOverride()
{
  return detail::LayoutClassOverride().load();
}

inline const char* LayoutClassName(LayoutClass layout_class)
{
  return layout_class == LayoutClass::Nintendo ? "nintendo" : "positional";
}
}  // namespace ciface::SDL
