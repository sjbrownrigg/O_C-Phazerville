// Copyright (c) 2026, Phazerville contributors
//
// Grids mode based on the Grids pattern generator, Copyright 2011 Émilie Gillet.
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
// EuclidKit - a 4-track drum sequencer with two switchable engines:
//   * Euclidean: each track its own length / beats / rotation (polymetric),
//     with per-step manual overrides.
//   * Grids: a 4-channel take on Mutable Instruments Grids -- a shared X/Y map
//     position + chaos drives 4 tracks, each reading one of the 3 Grids parts
//     at its own density.
// TR1 = master clock, TR2 = reset. CV1-4 modulate per-track (target selectable).

#pragma once

#include "../src/extern/bjorklund.h"
#ifdef DRUMMAP_GRIDS2
#include "../grids2_resources.h"
#else
#include "../grids_resources.h"
#endif

enum EuclidKitMode { MODE_EUCLID, MODE_GRIDS };

// CV modulation targets. Only a mode-appropriate subset is selectable at a
// time (see EuclidKit::CVTargetList).
enum CVTarget {
  CV_TARGET_NONE,
  CV_TARGET_BEATS,       // Euclidean
  CV_TARGET_ROTATION,    // Euclidean
  CV_TARGET_DENSITY,     // Grids (per-track fill)
  CV_TARGET_X,           // Grids (global map X)
  CV_TARGET_Y,           // Grids (global map Y)
  CV_TARGET_CHAOS,       // Grids (global)
  CV_TARGET_PROBABILITY, // both
  CV_TARGET_LAST
};
static const char * const kCVTargetNames[CV_TARGET_LAST] =
  {"None", "Beats", "Rotate", "Densty", "X", "Y", "Chaos", "Prob"};

enum DrumTrackSetting {
  TRACK_SETTING_LENGTH,
  TRACK_SETTING_BEATS,
  TRACK_SETTING_ROTATION,
  TRACK_SETTING_MANUAL_MASK,
  TRACK_SETTING_MUTE,
  TRACK_SETTING_PROBABILITY,
  TRACK_SETTING_CV_TARGET,
  TRACK_SETTING_FILL,   // Grids density, 0-255
  TRACK_SETTING_PART,   // Grids part: 0=BD 1=SD 2=HH
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
    { 100, 0, 100, "Prob", NULL, settings::STORAGE_TYPE_U8},
    { CV_TARGET_NONE, CV_TARGET_NONE, CV_TARGET_LAST - 1, "CV Trg", NULL, settings::STORAGE_TYPE_U8},
    { 128, 0, 255, "Fill", NULL, settings::STORAGE_TYPE_U8},
    { 0, 0, 2, "Part", NULL, settings::STORAGE_TYPE_U8},
  }};

  uint8_t length() const { return get_value(TRACK_SETTING_LENGTH); }
  uint8_t beats() const { return get_value(TRACK_SETTING_BEATS); }
  uint8_t rotation() const { return get_value(TRACK_SETTING_ROTATION); }
  uint16_t manual_mask() const { return get_value(TRACK_SETTING_MANUAL_MASK); }
  bool mute() const { return get_value(TRACK_SETTING_MUTE) != 0; }
  uint8_t probability() const { return get_value(TRACK_SETTING_PROBABILITY); }
  uint8_t cv_target() const { return get_value(TRACK_SETTING_CV_TARGET); }
  uint8_t fill() const { return get_value(TRACK_SETTING_FILL); }
  uint8_t part() const { return get_value(TRACK_SETTING_PART); }

  // Runtime playback state -- not persisted directly.
  uint16_t pattern = 0; // Euclidean step mask
  uint8_t step = 0;     // Euclidean per-track playhead
  uint8_t live_probability = 100; // CV-modulated per Controller() tick

  // Recompute the Euclidean pattern from Length/Beats/Rotation, preserving any
  // manually-toggled steps (TRACK_SETTING_MANUAL_MASK). Beats/Rotation can be
  // overridden (e.g. CV-modulated) without touching the stored settings.
  void Regenerate(int beats_override = -1, int rotation_override = -1) {
    uint8_t len = length();
    int b = (beats_override >= 0) ? beats_override : (int)beats();
    b = constrain(b, 0, (int)len);
    int rot = (rotation_override >= 0) ? rotation_override : (int)rotation();
    rot = ((rot % len) + len) % len;

    uint16_t euclid = (uint16_t)EuclideanPattern(len, (uint8_t)b, (uint8_t)rot);
    uint16_t mask = manual_mask();
    pattern = (euclid & ~mask) | (pattern & mask);

    if (step >= len) step = 0;
  }

  void ToggleStep(uint8_t i) {
    if (i >= 16) return;
    pattern ^= (uint16_t)(1 << i);
    apply_value(TRACK_SETTING_MANUAL_MASK, manual_mask() | (1 << i));
  }

  void ClearManual() {
    apply_value(TRACK_SETTING_MANUAL_MASK, 0);
    Regenerate();
  }

  void CopySettingsTo(DrumTrack &dst) const {
    for (int s = 0; s < TRACK_SETTING_LAST; ++s)
      dst.apply_value(s, get_value(s));
  }

  // Euclidean-mode step advance. Returns true if the pre-advance step is a hit
  // that should fire, subject to live_probability.
  bool Advance() {
    bool hit = false;
    if (!mute() && ((pattern >> step) & 0x1)) {
      hit = (live_probability >= 100) || ((uint32_t)random(0, 100) < live_probability);
    }
    step = (step + 1) % length();
    return hit;
  }
};
SETTINGS_ARRAY_DEFINE(DrumTrack);

// App-global settings, snapshotted alongside the tracks into each preset slot.
enum GlobalSetting {
  GLOBAL_SETTING_MODE,
  GLOBAL_SETTING_X,
  GLOBAL_SETTING_Y,
  GLOBAL_SETTING_CHAOS,
  GLOBAL_SETTING_LAST
};

class GridGlobals : public settings::SettingsBase<GridGlobals, GLOBAL_SETTING_LAST> {
public:
  SETTINGS_ARRAY_DECLARE() {{
    { MODE_EUCLID, MODE_EUCLID, MODE_GRIDS, "Mode", NULL, settings::STORAGE_TYPE_U8},
    { 128, 0, 255, "X", NULL, settings::STORAGE_TYPE_U8},
    { 128, 0, 255, "Y", NULL, settings::STORAGE_TYPE_U8},
    { 0, 0, 255, "Chaos", NULL, settings::STORAGE_TYPE_U8},
  }};
  uint8_t mode() const { return get_value(GLOBAL_SETTING_MODE); }
  uint8_t x() const { return get_value(GLOBAL_SETTING_X); }
  uint8_t y() const { return get_value(GLOBAL_SETTING_Y); }
  uint8_t chaos() const { return get_value(GLOBAL_SETTING_CHAOS); }
  void CopySettingsTo(GridGlobals &dst) const {
    for (int s = 0; s < GLOBAL_SETTING_LAST; ++s) dst.apply_value(s, get_value(s));
  }
};
SETTINGS_ARRAY_DEFINE(GridGlobals);

// Footer grouping of cursor fields into pages, for layout + page indicator.
enum EuclidKitPage { PAGE_MAIN, PAGE_MOD, PAGE_GLOBAL, PAGE_MODE, PAGE_CAL };

// The full set of editable fields. Which are active (and in what order) depends
// on the current mode -- see EuclidKit::FieldList().
enum Field {
  F_LENGTH, F_BEATS, F_ROTATION, // Euclidean main
  F_FILL, F_PART,                // Grids main
  F_MUTE,                        // main (both)
  F_PROB, F_CV,                  // mod (both)
  F_MODE,                        // engine (both)
  F_X, F_Y, F_CHAOS,             // global (Grids)
  F_CAL_LO, F_CAL_HI,            // CV input calibration (both)
  F_LAST
};

static constexpr int NUM_PRESETS = 4;
static const char * const kModeAbbrev[2] = {"EUC", "GRD"};
static const char * const kModeNames[2] = {"Euclid", "Grids"};
static const char * const kPartNames[3] = {"BD", "SD", "HH"};

OC_APP_CLASS(EuclidKit, TWOCCS("EK"), "EuclidKit", "4-Trk Euclid"),
  public HSApplication {
public:
  // Persisted: live state (4 tracks + globals) + NUM_PRESETS snapshot slots,
  // plus the two app-level CV calibration values (not part of preset snapshots).
  OC_APP_INTERFACE_DECLARE(EuclidKit,
    (1 + NUM_PRESETS) * (4 * DrumTrack::storageSize() + GridGlobals::storageSize())
    + 2 * sizeof(int32_t));

  DrumTrack tracks[4];
  GridGlobals globals;
  DrumTrack preset_tracks[NUM_PRESETS][4];
  GridGlobals preset_globals[NUM_PRESETS];

  void Start();

  void SaveSlot(int slot);
  void LoadSlot(int slot);

  void Resume() {}

  void Controller() {
    if (globals.mode() == MODE_GRIDS) ControllerGrids();
    else ControllerEuclid();
  }

  void ControllerEuclid() {
    // CVn modulates track n around its knob value; stored settings untouched.
    for (int i = 0; i < 4; ++i) {
      DrumTrack &t = tracks[i];
      int len = t.length();
      int mod_beats = t.beats();
      int mod_rotation = t.rotation();
      int mod_prob = t.probability();

      switch (t.cv_target()) {
        case CV_TARGET_BEATS:       mod_beats = MapCV(i, 0, len); break;
        case CV_TARGET_ROTATION:    mod_rotation = MapCV(i, 0, len > 0 ? len - 1 : 0); break;
        case CV_TARGET_PROBABILITY: mod_prob = MapCV(i, 0, 100); break;
        default: break;
      }
      t.live_probability = (uint8_t)mod_prob;
      live_beats[i] = mod_beats;
      live_rotation[i] = mod_rotation;
      t.Regenerate(mod_beats, mod_rotation);
    }

    if (Clock(1)) for (auto &t : tracks) t.step = 0; // TR2 reset
    if (Clock(0)) {                                  // TR1 clock
      for (int ch = 0; ch < 4; ++ch)
        if (tracks[ch].Advance()) ClockOut(ch);
    }
  }

  void ControllerGrids() {
    // Resolve global X/Y/Chaos + per-track density/probability, applying CV.
    live_x = globals.x();
    live_y = globals.y();
    int gchaos = globals.chaos();
    for (int i = 0; i < 4; ++i) {
      DrumTrack &t = tracks[i];
      live_density[i] = t.fill();
      int prob = t.probability();
      switch (t.cv_target()) {
        case CV_TARGET_DENSITY:     live_density[i] = MapCV(i, 0, 255); break;
        case CV_TARGET_X:           live_x = MapCV(i, 0, 255); break;
        case CV_TARGET_Y:           live_y = MapCV(i, 0, 255); break;
        case CV_TARGET_CHAOS:       gchaos = MapCV(i, 0, 255); break;
        case CV_TARGET_PROBABILITY: prob = MapCV(i, 0, 100); break;
        default: break;
      }
      t.live_probability = (uint8_t)prob;
    }
    live_chaos = gchaos;

    if (Clock(1)) shared_step = 0; // TR2 reset
    if (Clock(0)) {                // TR1 clock
      if (shared_step == 0) {
        int r = gchaos >> 2;
        for (int p = 0; p < 3; ++p) randomness[p] = r > 0 ? random(0, r) : 0;
      }
      for (int ch = 0; ch < 4; ++ch) {
        DrumTrack &t = tracks[ch];
        int level = ReadDrumMap(shared_step, t.part(), (uint8_t)live_x, (uint8_t)live_y);
        level = constrain(level + randomness[t.part()], 0, 255);
        bool hit = !t.mute() && level > (255 - live_density[ch]);
        if (hit && (t.live_probability >= 100 ||
                    (uint32_t)random(0, 100) < t.live_probability))
          ClockOut(ch);
      }
      shared_step = (shared_step + 1) % 32;
    }
  }

  void View() const;

  // -- Buttons --------------------------------------------------------------

  void OnUpButtonLongPress();
  void OnUpButtonPress();
  void OnDownButtonPress();
  void OnDownButtonLongPress();

  void OnLeftButtonPress();
  void OnLeftButtonLongPress();

  void OnRightButtonPress();

  // -- Encoders -------------------------------------------------------------

  void OnLeftEncoderMove(int direction);

  void OnRightEncoderMove(int direction);

private:
  int focus = 0;
  int cursor = 0;
  uint8_t grid_cursor = 0;

  bool preset_mode = false;
  int slot_cursor = 0;
  int active_slot = 0;
  mutable int save_flash = 0;
  static constexpr int kSaveFlashFrames = 12;
  static constexpr int kBigStep = 4; // encoder step for 0-255 params

  // Grids runtime state.
  uint8_t shared_step = 0;
  uint8_t randomness[3] = {0, 0, 0};
  int live_x = 128, live_y = 128;
  int live_density[4] = {128, 128, 128, 128};

  // Effective (post-CV) values stashed each Controller tick, so the UI can
  // show what's actually driving the sequence -- live confirmation of CV.
  // When a param isn't the track's CV target these equal the stored knob value.
  int live_beats[4] = {0, 0, 0, 0};
  int live_rotation[4] = {0, 0, 0, 0};
  int live_chaos = 0;

  // CV input calibration: the raw input values (pitch units, 1V = 12<<7)
  // captured at the low and high ends of the user's CV source. CV modulation
  // maps this span absolutely onto each target's own range. ~0V .. ~6V default.
  int cv_cal_low = 0;
  int cv_cal_high = 6 * (12 << 7);

  static constexpr char kTrackLetters[4] = {'A', 'B', 'C', 'D'};

  bool IsGrids() const { return globals.mode() == MODE_GRIDS; }

  // Active cursor field list for the current mode (in scroll order).
  const Field* FieldList(int &count) const {
    static const Field euclid[] =
      {F_LENGTH, F_BEATS, F_ROTATION, F_MUTE, F_PROB, F_CV,
       F_CAL_LO, F_CAL_HI, F_MODE};
    static const Field grids[] =
      {F_FILL, F_PART, F_MUTE, F_PROB, F_CV, F_X, F_Y, F_CHAOS,
       F_CAL_LO, F_CAL_HI, F_MODE};
    if (IsGrids()) { count = 11; return grids; }
    count = 9; return euclid;
  }
  Field CurrentField() const {
    int n; const Field* f = FieldList(n);
    return f[constrain(cursor, 0, n - 1)];
  }
  int IndexOfField(Field want) const {
    int n; const Field* f = FieldList(n);
    for (int i = 0; i < n; ++i) if (f[i] == want) return i;
    return 0;
  }

  // Map the calibrated CV input span absolutely onto [pmin, pmax].
  int MapCV(int ch, int pmin, int pmax) const {
    int span = cv_cal_high - cv_cal_low;
    if (span < 1) span = 1;
    long v = (long)(In(ch) - cv_cal_low) * (pmax - pmin) / span + pmin;
    return constrain((int)v, pmin, pmax);
  }
  int PageOf(Field f) const {
    switch (f) {
      case F_PROB: case F_CV: return PAGE_MOD;
      case F_X: case F_Y: case F_CHAOS: return PAGE_GLOBAL;
      case F_MODE: return PAGE_MODE;
      case F_CAL_LO: case F_CAL_HI: return PAGE_CAL;
      default: return PAGE_MAIN;
    }
  }

  // Mode-filtered CV target list.
  const uint8_t* CVTargetList(int &count) const {
    static const uint8_t euclid[] =
      {CV_TARGET_NONE, CV_TARGET_BEATS, CV_TARGET_ROTATION, CV_TARGET_PROBABILITY};
    static const uint8_t grids[] =
      {CV_TARGET_NONE, CV_TARGET_DENSITY, CV_TARGET_X, CV_TARGET_Y,
       CV_TARGET_CHAOS, CV_TARGET_PROBABILITY};
    if (IsGrids()) { count = 6; return grids; }
    count = 4; return euclid;
  }
  void AdjustCVTarget(DrumTrack &t, int dir);
  // On mode change, drop CV targets not valid for the new mode, clamp cursor.
  void OnModeChanged();

  // Grids bilinear interpolation of the drum map (from the DrumMap applet).
  uint8_t ReadDrumMap(uint8_t step, uint8_t part, uint8_t x, uint8_t y) const {
    uint8_t i = x >> 6;
    uint8_t j = y >> 6;
    const uint8_t* a_map = grids::drum_map[i][j];
    const uint8_t* b_map = grids::drum_map[i + 1][j];
    const uint8_t* c_map = grids::drum_map[i][j + 1];
    const uint8_t* d_map = grids::drum_map[i + 1][j + 1];
    uint8_t offset = (part * 32) + step;
    uint8_t a = a_map[offset], b = b_map[offset], c = c_map[offset], d = d_map[offset];
    uint8_t quad_x = x << 2;
    uint8_t quad_y = y << 2;
    uint8_t ab_fade = (b * quad_x + a * (255 - quad_x)) >> 8;
    uint8_t cd_fade = (d * quad_x + c * (255 - quad_x)) >> 8;
    return (cd_fade * quad_y + ab_fade * (255 - quad_y)) >> 8;
  }

  // Static Grids-hit preview for the track rows (uses stored X/Y/fill, no chaos).
  bool GridsHit(int ch, uint8_t s) const {
    const DrumTrack &t = tracks[ch];
    int level = ReadDrumMap(s, t.part(), globals.x(), globals.y());
    return level > (255 - (int)t.fill());
  }

  void DrawTrackRow(int i) const;

  void DrawFieldCursor(int x, int y, int w) const;
  // Box the field at (x,w) if it is the current cursor field.
  void CursorIf(Field f, int x, int y, int w) const;

  void DrawParams() const;

  // Grids global page (Euclidean mode has no global page).
  void DrawGlobalFooter(int y) const;

  // CV input calibration ("learn"): shows the live input voltage and the
  // selected captured endpoint. Left-encoder press grabs the current input.
  void DrawCalFooter(int y) const;

  void DrawPresetBar() const;
};

FLASHMEM void EuclidKit::DrawPresetBar() const {
  const int y = 52;
  gfxLine(0, y - 2, 127, y - 2);

  gfxPrint(1, y, "Kit");

  const int x0 = 22;
  const int pitch = 12;
  for (int p = 0; p < NUM_PRESETS; ++p) {
    int x = x0 + p * pitch;
    gfxPrint(x, y, p + 1);
    if (p == active_slot) gfxRect(x + 1, y - 3, 3, 2);
    if (p == slot_cursor) gfxFrame(x - 2, y - 1, 9, 9);
  }

  // Hints: UP = Save, DOWN = Load (left-encoder press exits).
  if (save_flash > 0) {
    gfxPrint(96, y, "SAVED");
    --save_flash;
  } else {
    // The up/down glyphs sit in the top/bottom of their boxes; nudge so both
    // arrowheads line up on the row's vertical middle, aligned with the text.
    gfxIcon(72, y + 1, UP_BTN_ICON); gfxPrint(80, y, "Sv");
    gfxIcon(98, y - 2, DOWN_BTN_ICON); gfxPrint(106, y, "Ld");
  }
}

FLASHMEM void EuclidKit::DrawCalFooter(int y) const {
  gfxPrint(1, y, "In"); gfxPos(15, y); gfxPrintVoltage(In(focus));
  const bool hi = (CurrentField() == F_CAL_HI);
  gfxPrint(64, y, hi ? "Hi" : "Lo");
  gfxPos(78, y); gfxPrintVoltage(hi ? cv_cal_high : cv_cal_low);
  DrawFieldCursor(64, y, 52); // selected endpoint; press to grab input
}

FLASHMEM void EuclidKit::DrawGlobalFooter(int y) const {
  gfxPrint(4, y, "X"); gfxPrint(12, y, live_x);
  CursorIf(F_X, 4, y, 26);
  gfxPrint(48, y, "Y"); gfxPrint(56, y, live_y);
  CursorIf(F_Y, 48, y, 26);
  gfxPrint(92, y, "Ch"); gfxPrint(104, y, live_chaos);
  CursorIf(F_CHAOS, 92, y, 32);
}

FLASHMEM void EuclidKit::DrawParams() const {
  const int y = 52;
  gfxLine(0, y - 2, 127, y - 2);

  const int page = PageOf(CurrentField());
  if (page == PAGE_GLOBAL) { DrawGlobalFooter(y); return; }
  if (page == PAGE_CAL) { DrawCalFooter(y); return; }
  if (page == PAGE_MODE) {
    gfxPrint(1, y, "Engine:"); gfxPrint(46, y, kModeNames[globals.mode()]);
    CursorIf(F_MODE, 46, y, 40);
    return;
  }

  char label[6] = {'T', 'r', 'k', kTrackLetters[focus], 0, 0};
  gfxPrint(1, y, label);
  const DrumTrack &t = tracks[focus];

  // Values shown are the effective (post-CV) values -- live_* equal the
  // stored knob value unless a CV input is modulating that param.
  if (page == PAGE_MOD) {
    gfxPrint(26, y, "Pr"); gfxPrint(38, y, (int)t.live_probability);
    CursorIf(F_PROB, 26, y, 24);
    gfxPrint(60, y, "CV:"); gfxPrint(78, y, kCVTargetNames[t.cv_target()]);
    CursorIf(F_CV, 60, y, 40);
    return;
  }

  // PAGE_MAIN
  if (IsGrids()) {
    gfxPrint(28, y, "F"); gfxPrint(34, y, live_density[focus]);
    CursorIf(F_FILL, 28, y, 24);
    gfxPrint(60, y, "Pt"); gfxPrint(74, y, kPartNames[t.part()]);
    CursorIf(F_PART, 60, y, 28);
    gfxPrint(96, y, "Mu"); gfxPrint(108, y, t.mute() ? "on" : "--");
    CursorIf(F_MUTE, 96, y, 24);
    return;
  }

  gfxPrint(28, y, "L"); gfxPrint(34, y, (int)t.length());
  CursorIf(F_LENGTH, 28, y, 18);
  gfxPrint(50, y, "B"); gfxPrint(56, y, live_beats[focus]);
  CursorIf(F_BEATS, 50, y, 18);
  gfxPrint(72, y, "R"); gfxPrint(78, y, live_rotation[focus]);
  CursorIf(F_ROTATION, 72, y, 18);
  gfxPrint(96, y, "Mu"); gfxPrint(108, y, t.mute() ? "on" : "--");
  CursorIf(F_MUTE, 96, y, 24);
}

FLASHMEM void EuclidKit::CursorIf(Field f, int x, int y, int w) const {
  if (CurrentField() == f) DrawFieldCursor(x, y, w);
}

FLASHMEM void EuclidKit::DrawFieldCursor(int x, int y, int w) const {
  if (EditMode()) gfxInvert(x - 1, y - 1, w + 2, 9);
  else gfxFrame(x - 1, y - 1, w + 2, 9);
}

FLASHMEM void EuclidKit::DrawTrackRow(int i) const {
  const int y = 12 + i * 10;
  const DrumTrack &t = tracks[i];

  char label[2] = {kTrackLetters[i], 0};
  gfxPrint(0, y, label);
  if (i == focus) gfxInvert(0, y - 1, 7, 9);
  if (t.mute()) gfxIcon(120, y, ZAP_ICON);

  const int x0 = 10;
  const int span = 116;

  if (IsGrids()) {
    const int steps = 32;
    const int pitch = span / steps; // ~3px
    for (int s = 0; s < steps; ++s) {
      int x = x0 + s * pitch;
      if (GridsHit(i, s)) gfxRect(x, y, 2, 5);
      else gfxPixel(x, y + 4);
      if (s == shared_step) gfxRect(x, y - 3, 2, 2);
    }
    return;
  }

  const uint8_t len = t.length();
  int pitch = span / len;
  if (pitch < 2) pitch = 2;
  for (uint8_t s = 0; s < len; ++s) {
    int x = x0 + s * pitch;
    bool on = (t.pattern >> s) & 0x1;
    if (on) gfxRect(x, y, 3, 5);
    else gfxFrame(x, y, 3, 5);
    if (s == t.step) gfxRect(x, y - 3, 3, 2);
    if (i == focus && s == grid_cursor) gfxFrame(x - 1, y - 1, 5, 7);
  }
}

FLASHMEM void EuclidKit::OnModeChanged() {
  int n; const uint8_t* list = CVTargetList(n);
  for (auto &t : tracks) {
    bool ok = false;
    for (int k = 0; k < n; ++k) if (list[k] == t.cv_target()) ok = true;
    if (!ok) t.apply_value(TRACK_SETTING_CV_TARGET, CV_TARGET_NONE);
  }
  int fc; FieldList(fc);
  if (cursor >= fc) cursor = fc - 1;
  shared_step = 0;
  for (auto &t : tracks) t.Regenerate();
  ResetCursor();
}

FLASHMEM void EuclidKit::AdjustCVTarget(DrumTrack &t, int dir) {
  int n; const uint8_t* list = CVTargetList(n);
  int idx = 0;
  for (int k = 0; k < n; ++k) if (list[k] == t.cv_target()) { idx = k; break; }
  idx = constrain(idx + dir, 0, n - 1);
  t.apply_value(TRACK_SETTING_CV_TARGET, list[idx]);
}

FLASHMEM void EuclidKit::OnRightEncoderMove(int direction) {
  if (preset_mode || IsGrids()) return; // step cursor: Euclidean only
  uint8_t len = tracks[focus].length();
  grid_cursor = (grid_cursor + direction + len) % len;
}

FLASHMEM void EuclidKit::OnLeftEncoderMove(int direction) {
  if (preset_mode) {
    slot_cursor = (slot_cursor + direction + NUM_PRESETS) % NUM_PRESETS;
    ResetCursor();
    return;
  }
  int n; FieldList(n);
  if (!EditMode()) {
    cursor = (cursor + direction + n) % n;
    ResetCursor();
    return;
  }

  DrumTrack &t = tracks[focus];
  switch (CurrentField()) {
    case F_LENGTH:
      t.change_value(TRACK_SETTING_LENGTH, direction);
      t.Regenerate();
      if (grid_cursor >= t.length()) grid_cursor = t.length() - 1;
      break;
    case F_BEATS:    t.change_value(TRACK_SETTING_BEATS, direction); t.Regenerate(); break;
    case F_ROTATION: t.change_value(TRACK_SETTING_ROTATION, direction); t.Regenerate(); break;
    case F_MUTE:     t.change_value(TRACK_SETTING_MUTE, direction); break;
    case F_FILL:     t.change_value(TRACK_SETTING_FILL, direction * kBigStep); break;
    case F_PART:     t.change_value(TRACK_SETTING_PART, direction); break;
    case F_PROB:     t.change_value(TRACK_SETTING_PROBABILITY, direction); break;
    case F_CV:       AdjustCVTarget(t, direction); break;
    case F_X:     globals.change_value(GLOBAL_SETTING_X, direction * kBigStep); break;
    case F_Y:     globals.change_value(GLOBAL_SETTING_Y, direction * kBigStep); break;
    case F_CHAOS: globals.change_value(GLOBAL_SETTING_CHAOS, direction * kBigStep); break;
    case F_MODE: {
      globals.change_value(GLOBAL_SETTING_MODE, direction);
      OnModeChanged();
      cursor = IndexOfField(F_MODE); // stay on the Engine field (and keep editing)
      break;
    }
    default: break;
  }
}

FLASHMEM void EuclidKit::OnRightButtonPress() { // toggle step (Euclidean only)
  if (preset_mode || IsGrids()) return;
  tracks[focus].ToggleStep(grid_cursor);
}

FLASHMEM void EuclidKit::OnLeftButtonLongPress() {
  if (preset_mode) return;
  tracks[focus].apply_value(TRACK_SETTING_MUTE, !tracks[focus].mute());
}

FLASHMEM void EuclidKit::OnLeftButtonPress() {
  if (preset_mode) { preset_mode = false; return; } // encoder press exits
  // On the calibration page, press captures the current input into the
  // selected endpoint (learn) rather than toggling edit mode.
  Field f = CurrentField();
  if (f == F_CAL_LO) { cv_cal_low = In(focus); ResetCursor(); return; }
  if (f == F_CAL_HI) { cv_cal_high = In(focus); ResetCursor(); return; }
  CursorToggle();
}

FLASHMEM void EuclidKit::OnDownButtonLongPress() { // clear manual overrides (Euclidean, not preset)
  if (preset_mode || IsGrids()) return;
  tracks[focus].ClearManual();
}

FLASHMEM void EuclidKit::OnDownButtonPress() { // preset screen: DOWN = Load
  if (preset_mode) { LoadSlot(slot_cursor); active_slot = slot_cursor; return; }
  focus = (focus + 1) & 3;
  ResetCursor();
}

FLASHMEM void EuclidKit::OnUpButtonPress() { // preset screen: UP = Save
  if (preset_mode) {
    SaveSlot(slot_cursor);
    active_slot = slot_cursor;
    save_flash = kSaveFlashFrames;
    return;
  }
  focus = (focus + 3) & 3;
  ResetCursor();
}

FLASHMEM void EuclidKit::OnUpButtonLongPress() { // toggle preset screen
  preset_mode = !preset_mode;
  if (preset_mode) slot_cursor = active_slot;
  CancelEdit();
  ResetCursor();
}

FLASHMEM void EuclidKit::LoadSlot(int slot) {
  preset_globals[slot].CopySettingsTo(globals);
  for (int i = 0; i < 4; ++i) {
    preset_tracks[slot][i].CopySettingsTo(tracks[i]);
    tracks[i].Regenerate();
  }
  OnModeChanged();
  CancelEdit();
  cursor = 0;
}

FLASHMEM void EuclidKit::SaveSlot(int slot) {
  globals.CopySettingsTo(preset_globals[slot]);
  for (int i = 0; i < 4; ++i) tracks[i].CopySettingsTo(preset_tracks[slot][i]);
}

FLASHMEM void EuclidKit::Start() {
  // Sentinel: freshly-constructed app has all-zero settings (invalid Length).
  // Seed defaults once; InitDefaults() applies every declared default.
  if (!tracks[0].length()) {
    static const uint8_t def_len[4]   = {16, 16, 16, 13};
    static const uint8_t def_beats[4] = { 4,  2,  8,  5};
    static const uint8_t def_rot[4]   = { 0,  4,  0,  0};
    static const uint8_t def_part[4]  = { 0,  1,  2,  2}; // BD SD HH HH
    globals.InitDefaults();
    for (int i = 0; i < 4; ++i) {
      tracks[i].InitDefaults();
      tracks[i].apply_value(TRACK_SETTING_LENGTH, def_len[i]);
      tracks[i].apply_value(TRACK_SETTING_BEATS, def_beats[i]);
      tracks[i].apply_value(TRACK_SETTING_ROTATION, def_rot[i]);
      tracks[i].apply_value(TRACK_SETTING_PART, def_part[i]);
    }
    // Seed every preset slot with the default kit.
    for (int p = 0; p < NUM_PRESETS; ++p) {
      globals.CopySettingsTo(preset_globals[p]);
      for (int i = 0; i < 4; ++i) tracks[i].CopySettingsTo(preset_tracks[p][i]);
    }
  }
  for (auto &t : tracks) t.Regenerate();
}

FLASHMEM void EuclidKit::View() const {
  gfxHeader("EuclidKit");
  gfxPrint(92, 1, kModeAbbrev[globals.mode()]);
  gfxIcon(118, 1, GAUGE_ICON);
  if (PageOf(CurrentField()) != PAGE_MAIN) gfxInvert(117, 0, 10, 9);

  for (int i = 0; i < 4; ++i) DrawTrackRow(i);
  if (preset_mode) DrawPresetBar();
  else DrawParams();
}


constexpr char EuclidKit::kTrackLetters[4];

// App stubs
FLASHMEM void EuclidKit::Init() { BaseStart(); }

void EuclidKit::Process(OC::IOFrame *ioframe) { BaseController(ioframe); }

FLASHMEM void EuclidKit::GetIOConfig(OC::IOConfig &ioconfig) const {
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

FLASHMEM size_t EuclidKit::SaveAppData(util::StreamBufferWriter &stream_buffer) const {
  for (auto &t : tracks) t.Save(stream_buffer);
  globals.Save(stream_buffer);
  for (int p = 0; p < NUM_PRESETS; ++p) {
    for (auto &t : preset_tracks[p]) t.Save(stream_buffer);
    preset_globals[p].Save(stream_buffer);
  }
  stream_buffer.Write((int32_t)cv_cal_low);
  stream_buffer.Write((int32_t)cv_cal_high);
  return stream_buffer.written();
}

FLASHMEM size_t EuclidKit::RestoreAppData(util::StreamBufferReader &stream_buffer) {
  for (auto &t : tracks) t.Restore(stream_buffer);
  globals.Restore(stream_buffer);
  for (int p = 0; p < NUM_PRESETS; ++p) {
    for (auto &t : preset_tracks[p]) t.Restore(stream_buffer);
    preset_globals[p].Restore(stream_buffer);
  }
  cv_cal_low = stream_buffer.Read<int32_t>();
  cv_cal_high = stream_buffer.Read<int32_t>();
  for (auto &t : tracks) t.Regenerate();
  return stream_buffer.read();
}

FLASHMEM void EuclidKit::HandleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Resume();
}

void EuclidKit::Loop() {}

FLASHMEM void EuclidKit::DrawMenu() const { BaseView(); }

FLASHMEM void EuclidKit::DrawScreensaver() const { BaseScreensaver(); }

void EuclidKit::DrawDebugInfo() const {}

FLASHMEM void EuclidKit::HandleButtonEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_BUTTON_L) {
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnLeftButtonLongPress();
    if (event.type == UI::EVENT_BUTTON_PRESS) OnLeftButtonPress();
  }
  if (event.control == OC::CONTROL_BUTTON_R && event.type == UI::EVENT_BUTTON_PRESS)
    OnRightButtonPress();
  if (event.control == OC::CONTROL_BUTTON_UP) {
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnUpButtonLongPress();
    if (event.type == UI::EVENT_BUTTON_PRESS) OnUpButtonPress();
  }
  if (event.control == OC::CONTROL_BUTTON_DOWN) {
    if (event.type == UI::EVENT_BUTTON_PRESS) OnDownButtonPress();
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnDownButtonLongPress();
  }
}

FLASHMEM void EuclidKit::HandleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoderMove(event.value);
}
