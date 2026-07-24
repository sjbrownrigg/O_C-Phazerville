// Copyright (c) 2026, Phazerville contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// EuclidKit - a 4-track polymetric Euclidean drum sequencer.
// Each of the 4 outputs is its own track with an independent step length,
// Euclidean fill/rotation, and per-step manual override.
// TR1 = master clock (advances all tracks), TR2 = reset all tracks to step 0.

#pragma once

#include "../src/extern/bjorklund.h"

enum DrumTrackSetting {
  TRACK_SETTING_LENGTH,
  TRACK_SETTING_BEATS,
  TRACK_SETTING_ROTATION,
  TRACK_SETTING_MANUAL_MASK,
  TRACK_SETTING_MUTE,
  TRACK_SETTING_LAST
};

class DrumTrack : public settings::SettingsBase<DrumTrack, TRACK_SETTING_LAST> {
public:
  SETTINGS_ARRAY_DECLARE() {{
    {16, 1, 16, "Length", NULL, settings::STORAGE_TYPE_U8},
    { 4, 0, 16, "Beats", NULL, settings::STORAGE_TYPE_U8},
    { 0, 0, 15, "Rotate", NULL, settings::STORAGE_TYPE_U8},
    { 0, 0, 65535, "Steps", NULL, settings::STORAGE_TYPE_U16},
    { 0, 0, 1, "Mute", NULL, settings::STORAGE_TYPE_U8},
  }};

  uint8_t length() const { return get_value(TRACK_SETTING_LENGTH); }
  uint8_t beats() const { return get_value(TRACK_SETTING_BEATS); }
  uint8_t rotation() const { return get_value(TRACK_SETTING_ROTATION); }
  uint16_t manual_mask() const { return get_value(TRACK_SETTING_MANUAL_MASK); }
  bool mute() const { return get_value(TRACK_SETTING_MUTE) != 0; }

  // Runtime playback state -- not persisted directly (derived from the
  // settings above on Regenerate(), except for manual step overrides).
  uint16_t pattern = 0;
  uint8_t step = 0;

  // Recompute the Euclidean pattern from Length/Beats/Rotation, while
  // preserving any steps the user has manually toggled (tracked in
  // TRACK_SETTING_MANUAL_MASK).
  void Regenerate() {
    uint8_t len = length();
    uint8_t b = beats();
    if (b > len) b = len;
    uint8_t rot = rotation() % len;

    uint16_t euclid = (uint16_t)EuclideanPattern(len, b, rot);
    uint16_t mask = manual_mask();
    pattern = (euclid & ~mask) | (pattern & mask);

    if (step >= len) step = 0;
  }

  // Flip a single step on/off and lock it so future Regenerate() calls
  // (from turning the Length/Beats/Rotation knobs) don't overwrite it.
  void ToggleStep(uint8_t i) {
    if (i >= 16) return;
    pattern ^= (uint16_t)(1 << i);
    apply_value(TRACK_SETTING_MANUAL_MASK, manual_mask() | (1 << i));
  }

  // Release all manual overrides and snap back to the pure Euclidean shape.
  void ClearManual() {
    apply_value(TRACK_SETTING_MANUAL_MASK, 0);
    Regenerate();
  }

  // Advance the playhead by one step, returning true if the *current*
  // step (before advancing) is a hit that should fire a trigger.
  bool Advance() {
    bool hit = !mute() && ((pattern >> step) & 0x1);
    step = (step + 1) % length();
    return hit;
  }
};
SETTINGS_ARRAY_DEFINE(DrumTrack);

enum EuclidKitCursor {
  DK_LENGTH,
  DK_BEATS,
  DK_ROTATION,
  DK_MUTE,
  DK_PARAM_LAST
};

OC_APP_CLASS(EuclidKit, TWOCCS("EK"), "EuclidKit", "4-Trk Euclid"),
  public HSApplication {
public:
  OC_APP_INTERFACE_DECLARE(EuclidKit, 4 * DrumTrack::storageSize());

  DrumTrack tracks[4];

  void Start() {
    // Sentinel: a freshly-constructed app has all-zero settings, which is
    // an invalid Length (min is 1). Only seed defaults the first time.
    if (!tracks[0].length()) {
      static const uint8_t def_len[4]   = {16, 16, 16, 13};
      static const uint8_t def_beats[4] = { 4,  2,  8,  5};
      static const uint8_t def_rot[4]   = { 0,  4,  0,  0};
      for (int i = 0; i < 4; ++i) {
        tracks[i].apply_value(TRACK_SETTING_LENGTH, def_len[i]);
        tracks[i].apply_value(TRACK_SETTING_BEATS, def_beats[i]);
        tracks[i].apply_value(TRACK_SETTING_ROTATION, def_rot[i]);
      }
    }
    for (auto &t : tracks) t.Regenerate();
  }

  void Resume() {}

  void Controller() {
    if (Clock(1)) { // TR2 - reset
      for (auto &t : tracks) t.step = 0;
    }
    if (Clock(0)) { // TR1 - master clock
      for (int ch = 0; ch < 4; ++ch) {
        if (tracks[ch].Advance()) ClockOut(ch);
      }
    }
  }

  void View() const {
    gfxHeader("EuclidKit");
    for (int i = 0; i < 4; ++i) DrawTrackRow(i);
    DrawParams();
  }

  void OnUpButtonPress() { focus = (focus + 3) & 3; ResetCursor(); }
  void OnDownButtonPress() { focus = (focus + 1) & 3; ResetCursor(); }
  void OnDownButtonLongPress() { tracks[focus].ClearManual(); }

  void OnLeftButtonPress() { CursorToggle(); }
  void OnLeftButtonLongPress() {
    tracks[focus].apply_value(TRACK_SETTING_MUTE, !tracks[focus].mute());
  }

  void OnRightButtonPress() {
    tracks[focus].ToggleStep(grid_cursor);
  }

  void OnLeftEncoderMove(int direction) {
    if (!EditMode()) {
      cursor = (cursor + direction + DK_PARAM_LAST) % DK_PARAM_LAST;
      ResetCursor();
      return;
    }

    DrumTrack &t = tracks[focus];
    switch (cursor) {
      case DK_LENGTH:
        t.change_value(TRACK_SETTING_LENGTH, direction);
        t.Regenerate();
        if (grid_cursor >= t.length()) grid_cursor = t.length() - 1;
        break;
      case DK_BEATS:
        t.change_value(TRACK_SETTING_BEATS, direction);
        t.Regenerate();
        break;
      case DK_ROTATION:
        t.change_value(TRACK_SETTING_ROTATION, direction);
        t.Regenerate();
        break;
      case DK_MUTE:
        t.change_value(TRACK_SETTING_MUTE, direction);
        break;
    }
  }

  void OnRightEncoderMove(int direction) {
    uint8_t len = tracks[focus].length();
    grid_cursor = (grid_cursor + direction + len) % len;
  }

private:
  int focus = 0;
  int cursor = 0;
  uint8_t grid_cursor = 0;

  static constexpr char kTrackLetters[4] = {'A', 'B', 'C', 'D'};

  void DrawTrackRow(int i) const {
    const int y = 12 + i * 10;
    const DrumTrack &t = tracks[i];
    const uint8_t len = t.length();

    char label[2] = {kTrackLetters[i], 0};
    gfxPrint(0, y, label);
    if (i == focus) gfxInvert(0, y - 1, 7, 9);
    if (t.mute()) gfxIcon(120, y, ZAP_ICON);

    const int x0 = 10;
    const int span = 116;
    int pitch = span / len;
    if (pitch < 2) pitch = 2;

    for (uint8_t s = 0; s < len; ++s) {
      int x = x0 + s * pitch;
      bool on = (t.pattern >> s) & 0x1;
      if (on) gfxRect(x, y, 3, 5);
      else gfxFrame(x, y, 3, 5);

      if (s == t.step) gfxRect(x, y - 3, 3, 2); // playhead tick, above the step
      if (i == focus && s == grid_cursor) gfxFrame(x - 1, y - 1, 5, 7);
    }
  }

  // A persistent selection indicator: a hollow box while just navigating,
  // an inverted (filled) box while actively editing the value. Brackets the
  // field's own text row directly, so it doesn't need blank space below.
  void DrawFieldCursor(int x, int y, int w) const {
    if (EditMode()) gfxInvert(x - 1, y - 1, w + 2, 9);
    else gfxFrame(x - 1, y - 1, w + 2, 9);
  }

  void DrawParams() const {
    const int y = 52;
    gfxLine(0, y - 2, 127, y - 2);
    const DrumTrack &t = tracks[focus];

    char label[6];
    label[0] = 'T'; label[1] = 'r'; label[2] = 'k'; label[3] = kTrackLetters[focus]; label[4] = 0;
    gfxPrint(1, y, label);

    gfxPrint(28, y, "L");
    gfxPrint(34, y, (int)t.length());
    if (cursor == DK_LENGTH) DrawFieldCursor(28, y, 18);

    gfxPrint(50, y, "B");
    gfxPrint(56, y, (int)t.beats());
    if (cursor == DK_BEATS) DrawFieldCursor(50, y, 18);

    gfxPrint(72, y, "R");
    gfxPrint(78, y, (int)t.rotation());
    if (cursor == DK_ROTATION) DrawFieldCursor(72, y, 18);

    gfxPrint(96, y, "Mu");
    gfxPrint(108, y, t.mute() ? "on" : "--");
    if (cursor == DK_MUTE) DrawFieldCursor(96, y, 24);
  }
};

constexpr char EuclidKit::kTrackLetters[4];

// App stubs
void EuclidKit::Init() { BaseStart(); }

void EuclidKit::Process(OC::IOFrame *ioframe) {
  BaseController(ioframe);
}

void EuclidKit::GetIOConfig(OC::IOConfig &ioconfig) const {
  using namespace OC;
  ioconfig.digital_inputs[DIGITAL_INPUT_1].set("Clock");
  ioconfig.digital_inputs[DIGITAL_INPUT_2].set("Reset");
  ioconfig.digital_inputs[DIGITAL_INPUT_3].set("");
  ioconfig.digital_inputs[DIGITAL_INPUT_4].set("");

  ioconfig.outputs[0].set("Trk A", OUTPUT_MODE_GATE);
  ioconfig.outputs[1].set("Trk B", OUTPUT_MODE_GATE);
  ioconfig.outputs[2].set("Trk C", OUTPUT_MODE_GATE);
  ioconfig.outputs[3].set("Trk D", OUTPUT_MODE_GATE);
}

size_t EuclidKit::SaveAppData(util::StreamBufferWriter &stream_buffer) const {
  for (auto &t : tracks) t.Save(stream_buffer);
  return stream_buffer.written();
}

size_t EuclidKit::RestoreAppData(util::StreamBufferReader &stream_buffer) {
  for (auto &t : tracks) t.Restore(stream_buffer);
  for (auto &t : tracks) t.Regenerate();
  return stream_buffer.read();
}

void EuclidKit::HandleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Resume();
}

void EuclidKit::Loop() {}

void EuclidKit::DrawMenu() const { BaseView(); }

void EuclidKit::DrawScreensaver() const { BaseScreensaver(); }

void EuclidKit::DrawDebugInfo() const {}

void EuclidKit::HandleButtonEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_BUTTON_L) {
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnLeftButtonLongPress();
    if (event.type == UI::EVENT_BUTTON_PRESS) OnLeftButtonPress();
  }

  if (event.control == OC::CONTROL_BUTTON_R && event.type == UI::EVENT_BUTTON_PRESS) OnRightButtonPress();

  if (event.control == OC::CONTROL_BUTTON_UP && event.type == UI::EVENT_BUTTON_PRESS) OnUpButtonPress();

  if (event.control == OC::CONTROL_BUTTON_DOWN) {
    if (event.type == UI::EVENT_BUTTON_PRESS) OnDownButtonPress();
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnDownButtonLongPress();
  }
}

void EuclidKit::HandleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoderMove(event.value);
}
