#!/usr/bin/env python3
"""Retheme the copied ModernGekko launcher for WWE WrestleMania X8."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "lib" / "ModernGekko-Template" / "lib" / "ModernGekko" / "tools" / "moderngekko_launcher.cpp"

BACKDROP = '''    // ------------------------------------------------------------------
    // Procedural WrestleMania arena backdrop (no game assets are shipped).
    // ------------------------------------------------------------------
    draw->AddRectFilledMultiColor(
        origin, ImVec2(origin.x + size.x, origin.y + size.y),
        IM_COL32(5, 6, 11, 255), IM_COL32(10, 10, 18, 255),
        IM_COL32(26, 16, 18, 255), IM_COL32(5, 6, 11, 255));

    // Gold spotlight beams from the arena rig.
    for (int beam = 0; beam < 5; ++beam)
    {
      const float x = origin.x + size.x * (0.14f + 0.18f * beam);
      const float sway = std::sin(static_cast<float>(now) * 0.22f + beam * 0.9f) * 12 * S;
      draw->AddTriangleFilled(
          ImVec2(x + sway, origin.y - 30 * S),
          ImVec2(x - 150 * S, origin.y + size.y * 0.88f),
          ImVec2(x + 190 * S, origin.y + size.y * 0.88f),
          IM_COL32(255, 214, 140, 9));
      draw->AddTriangleFilled(
          ImVec2(x + sway, origin.y - 30 * S),
          ImVec2(x - 60 * S, origin.y + size.y * 0.82f),
          ImVec2(x + 86 * S, origin.y + size.y * 0.82f),
          IM_COL32(255, 236, 195, 11));
    }

    // Crowd bokeh, slowly twinkling.
    for (int i = 0; i < 90; ++i)
    {
      const float fx = std::fmod(i * 0.6180339f, 1.0f);
      const float fy = std::fmod(i * 0.381966f + 0.06f, 0.40f);
      const float twinkle = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.8f + i * 1.7f);
      const int alpha = static_cast<int>(10 + 26 * twinkle);
      const ImU32 color = (i % 3 == 0) ? IM_COL32(245, 205, 130, alpha)
                          : (i % 3 == 1) ? IM_COL32(190, 200, 220, alpha)
                                         : IM_COL32(150, 60, 70, alpha);
      draw->AddCircleFilled(
          ImVec2(origin.x + size.x * fx, origin.y + size.y * fy),
          (1.2f + (i % 5) * 0.8f) * S, color);
    }

    // The ring: canvas, apron, three red ropes and corner posts.
    const float ring_cx = origin.x + size.x * 0.52f;
    const float ring_cy = origin.y + size.y * 0.73f;
    const float ring_rx = size.x * 0.33f;
    const float ring_ry = size.y * 0.11f;
    draw->AddEllipseFilled(ImVec2(ring_cx, ring_cy + ring_ry * 0.45f),
                           ImVec2(ring_rx, ring_ry * 0.85f),
                           IM_COL32(12, 13, 18, 150), 0, 64);
    draw->AddEllipse(ImVec2(ring_cx, ring_cy), ImVec2(ring_rx, ring_ry),
                     IM_COL32(78, 80, 92, 110), 0, 64, 4 * S);
    draw->AddEllipse(ImVec2(ring_cx, ring_cy), ImVec2(ring_rx, ring_ry),
                     IM_COL32(150, 28, 40, 120), 0, 64, 2 * S);
    for (int rope = 0; rope < 3; ++rope)
    {
      ImVec2 points[33];
      for (int seg = 0; seg <= 32; ++seg)
      {
        const float t = seg / 32.0f;
        const float px = ring_cx - ring_rx * 1.12f + ring_rx * 2.24f * t;
        const float py = ring_cy - ring_ry * (0.85f - rope * 0.42f) +
                         std::sin(t * 3.14159265f) * ring_ry * 0.32f;
        points[seg] = ImVec2(px, py);
      }
      draw->AddPolyline(points, 33, IM_COL32(185, 28, 42, 165), 0, 3.0f * S);
    }
    for (const float side : {-1.0f, 1.0f})
    {
      const float px = ring_cx + side * ring_rx * 1.12f;
      draw->AddRectFilled(ImVec2(px - 6 * S, ring_cy - ring_ry * 1.5f),
                          ImVec2(px + 6 * S, ring_cy + ring_ry * 0.6f),
                          IM_COL32(110, 114, 128, 90), 4 * S);
    }

'''

OLD_HEADER = '''    ImGui::PushFont(font_bold, 15 * S);
    ImGui::TextColored(ImVec4(0.98f, 0.85f, 0.20f, 1.0f), "SPONGEBOB SQUAREPANTS");
    ImGui::PopFont();
    ImGui::SetCursorPos(ImVec2(pad, 38 * S));
    ImGui::PushFont(font_bold, 32 * S);
    ImGui::TextUnformatted("Revenge of the Flying Dutchman");
    ImGui::PopFont();'''

NEW_HEADER = '''    ImGui::PushFont(font_bold, 15 * S);
    ImGui::TextColored(ImVec4(0.98f, 0.85f, 0.20f, 1.0f), "STATIC RECOMPILATION");
    ImGui::PopFont();
    ImGui::SetCursorPos(ImVec2(pad, 38 * S));
    ImGui::PushFont(font_bold, 32 * S);
    ImGui::TextUnformatted("WWE WrestleMania X8");
    ImGui::PopFont();'''

REPLACEMENTS = [
    # Header
    (OLD_HEADER, NEW_HEADER),
    # Chip and small labels
    ('game_ready ? "GSQE78  \x07  READY" : "NO GAME IMPORTED"',
     'game_ready ? "GW3E78  |  READY" : "NO GAME IMPORTED"'),
    ('"Static recompilation  \x07  PC"', '"Static recompilation  |  PC"'),
    ('"Disc ID %s  \x07  retail DOL verified"', '"Disc ID %s  |  retail DOL verified"'),
    # Runner name must follow the configured output name.
    ('"The game runner (SpongeBobRecompRunner.exe) is missing from this "\n'
     '                  "folder. Reinstall the app files next to the launcher.";',
     '"The game runner (" MODERNGEKKO_RUNNER_FILENAME ") is missing from this "\n'
     '                  "folder. Reinstall the app files next to the launcher.";'),
    # Palette: teal -> charcoal and wrestling red.
    ('style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.66f, 0.72f, 1.00f);',
     'style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.62f, 0.62f, 0.66f, 1.00f);'),
    ('style.Colors[ImGuiCol_ChildBg] = ImVec4(0.025f, 0.09f, 0.14f, 0.82f);',
     'style.Colors[ImGuiCol_ChildBg] = ImVec4(0.035f, 0.040f, 0.055f, 0.88f);'),
    ('style.Colors[ImGuiCol_PopupBg] = ImVec4(0.03f, 0.10f, 0.15f, 0.98f);',
     'style.Colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.045f, 0.06f, 0.98f);'),
    ('style.Colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.16f, 0.22f, 0.92f);',
     'style.Colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.08f, 0.11f, 0.92f);'),
    ('style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.24f, 0.31f, 0.95f);',
     'style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.10f, 0.12f, 0.95f);'),
    ('style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.13f, 0.30f, 0.38f, 0.98f);',
     'style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.12f, 0.14f, 0.98f);'),
    ('style.Colors[ImGuiCol_Button] = ImVec4(0.08f, 0.19f, 0.25f, 0.95f);',
     'style.Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.09f, 0.12f, 0.95f);'),
    ('style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.30f, 0.38f, 1.00f);',
     'style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.12f, 0.14f, 1.00f);'),
    ('style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.38f, 0.46f, 1.00f);',
     'style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.15f, 0.16f, 1.00f);'),
    ('style.Colors[ImGuiCol_Header] = ImVec4(0.12f, 0.26f, 0.33f, 1.00f);',
     'style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.10f, 0.12f, 1.00f);'),
    ('style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.16f, 0.34f, 0.42f, 1.00f);',
     'style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.12f, 0.14f, 1.00f);'),
    ('style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.42f, 0.50f, 1.00f);',
     'style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.15f, 0.16f, 1.00f);'),
    ('style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.40f, 0.48f, 0.80f);',
     'style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.20f, 0.20f, 0.80f);'),
    # Sidebar nav buttons.
    ('active ? ImVec4(0.12f, 0.26f, 0.32f, 0.95f)\n'
     '                                   : ImVec4(0.04f, 0.12f, 0.17f, 0.55f));',
     'active ? ImVec4(0.34f, 0.11f, 0.13f, 0.95f)\n'
     '                                   : ImVec4(0.06f, 0.07f, 0.10f, 0.60f));'),
    ('ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.24f, 0.30f, 0.95f));',
     'ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.10f, 0.12f, 0.95f));'),
    ('active ? ImVec4(0.99f, 0.87f, 0.28f, 1.0f)\n'
     '                                   : ImVec4(0.80f, 0.90f, 0.94f, 1.0f));',
     'active ? ImVec4(0.99f, 0.87f, 0.28f, 1.0f)\n'
     '                                   : ImVec4(0.86f, 0.87f, 0.90f, 1.0f));'),
    ('ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.66f, 0.72f, 1.0f));',
     'ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.62f, 0.66f, 1.0f));'),
    ('ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.025f, 0.090f, 0.135f, 0.86f));',
     'ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.035f, 0.040f, 0.055f, 0.90f));'),
]


def main() -> None:
    text = SRC.read_text(encoding="utf-8")

    start_marker = "    // ------------------------------------------------------------------\n    // Procedural underwater backdrop"
    end_marker = "    // ------------------------------------------------------------------\n    // Header"
    start = text.find(start_marker)
    if start < 0:
        raise SystemExit("backdrop start marker not found")
    end = text.find(end_marker, start)
    if end < 0:
        raise SystemExit("backdrop end marker not found")
    text = text[:start] + BACKDROP + text[end:]

    missing = []
    for old, new in REPLACEMENTS:
        if old not in text:
            missing.append(old.splitlines()[0])
            continue
        text = text.replace(old, new, 1)

    SRC.write_text(text, encoding="utf-8")
    if missing:
        print("WARNING: not found:")
        for item in missing:
            print("  " + item)
    print(f"patched {SRC}")


if __name__ == "__main__":
    main()
