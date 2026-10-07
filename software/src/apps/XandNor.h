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
// (X)AND (N)OR - four gate-length processors (channels CH1-CH4) feeding freely
// routed gate outputs. Each output carries a channel's gate, its inverse, or a
// logic function (AND, NAND, OR, NOR, XOR, XNOR) of two channels.
//
// Built for the Teensy 4.1 (ORN8) only: outputs A-H, CV inputs 1-8, matching
// the panel labels. (The Teensy 4.0 build has no room for its preset area.)
//
// Only Controller() runs in the 16.7 kHz ISR. Everything else is defined out of
// class and marked FLASHMEM: in-class definitions stay in RAM1 regardless.

#pragma once

namespace XandNorDefs {

enum Page {
  PAGE_OVERVIEW,
  PAGE_CHANNEL_A,
  PAGE_CHANNEL_B,
  PAGE_CHANNEL_C,
  PAGE_CHANNEL_D,
  PAGE_ROUTING,
  PAGE_COUNT
};

static constexpr int kNumChannels = 4;
static constexpr int kNumOutputs = DAC_CHANNEL_COUNT;

static const char * const kPageNames[PAGE_COUNT] = {
  "Overview", "CH1", "CH2", "CH3", "CH4", "Routing"
};

// Output functions. OUT and NOT read one channel; the rest combine two.
enum Function { FN_OUT, FN_NOT, FN_AND, FN_NAND, FN_OR, FN_NOR, FN_XOR, FN_XNOR, FN_COUNT };
static const char * const kFunctionNames[FN_COUNT] = {
  "OUT", "NOT", "AND", "NAND", "OR", "NOR", "XOR", "XNOR"
};

struct Route {
  uint8_t fn;
  uint8_t a, b; // channel indices; b unused by OUT and NOT
};

// Default patch: every channel on T1 with a different length, so one clock fans
// out into nested gates (outputs A-D) and the slices between them (E-H).
static const Route kDefaultRoutes[8] = {
  {FN_OUT, 0, 0}, {FN_OUT, 1, 1}, {FN_OUT, 2, 2}, {FN_OUT, 3, 3},
  {FN_XOR, 0, 1}, {FN_XOR, 1, 2}, {FN_XOR, 2, 3}, {FN_NOT, 3, 3},
};

// Gate lengths in microseconds: 1 ms to 10 s, ten steps per decade.
// Length setting 0 is Thru (follow the input gate); kLengthInfinite holds the
// gate high until the next trigger.
static constexpr uint32_t kLengthUs[] = {
  1000, 1200, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000,
  10000, 12000, 15000, 20000, 25000, 30000, 40000, 50000, 60000, 80000,
  100000, 120000, 150000, 200000, 250000, 300000, 400000, 500000, 600000, 800000,
  1000000, 1200000, 1500000, 2000000, 2500000, 3000000, 4000000, 5000000, 6000000, 8000000,
  10000000,
};
static constexpr int kNumLengths = sizeof(kLengthUs) / sizeof(kLengthUs[0]);
static constexpr int kLengthThru = 0;
static constexpr int kLengthInfinite = kNumLengths + 1;
static constexpr int kDefaultLengths[kNumChannels] = {11, 18, 21, 25}; // 10, 50, 100, 250 ms

// A retrigger while the gate is high drops it for this long, so downstream
// modules see a fresh edge. 4 ticks = 240 us.
static constexpr uint8_t kDropTicks = 4;

enum ChannelField { FIELD_SOURCE, FIELD_LENGTH, FIELD_CV, FIELD_DEPTH, FIELD_MODE, FIELD_COUNT };

// How CV modulation is read: once per trigger, or continuously.
enum CVMode : uint8_t { CV_SAMPLE, CV_TRACK, CV_MODE_COUNT };
static const char * const kCVModeNames[CV_MODE_COUNT] = {"Sample", "Track"};

// Length CV moves the length 8 steps per volt on the 10-steps-per-decade scale,
// so +5 V at 100% depth spans the whole 1 ms - 10 s range.
static constexpr int kStepsPerVolt = 8;

// Routing page cursor: each output has three fields, the second channel only
// for two-input functions.
enum RouteField { ROUTE_FN, ROUTE_A, ROUTE_B, ROUTE_FIELDS };

static constexpr bool TwoInputs(uint8_t fn) { return fn != FN_OUT && fn != FN_NOT; }

// A patch as stored in EEPROM and in the preset slots.
// Per-channel settings beyond source and length. The reserved bytes hold
// M5/M6 settings later without changing the stored size, which would make
// the platform discard saved data.
struct ChannelExtras {
  uint16_t length_cv; // CVInputMap::Pack()
  uint8_t cv_mode;
  uint8_t reserved[13];
};

struct PackedPatch {
  uint32_t source[kNumChannels]; // DigitalInputMap::Pack()
  uint8_t length[kNumChannels];
  Route routes[8];
  ChannelExtras extras[kNumChannels];
  uint8_t reserved[20];
};
static_assert(sizeof(PackedPatch) == 128, "PackedPatch layout changed");

static constexpr int kNumSlots = 8;
// Bump when PackedPatch changes; stored data with another version is ignored.
static constexpr uint8_t kStorageVersion = 2;
// version + slot-used mask + working patch + slots
static constexpr size_t kStorageSize = 2 + (1 + kNumSlots) * sizeof(PackedPatch);
static constexpr int kMessageFrames = 40; // "Saved"/"Loaded" banner, ~1 s

// Up/Down are labelled A/B on the Teensy 4.1 (ORN8) panel.
#ifdef ARDUINO_TEENSY41
static const char * const kPresetHint = "A:Save  B:Load";
#else
static const char * const kPresetHint = "Up:Save  Down:Load";
#endif

} // namespace XandNorDefs

OC_APP_CLASS(XandNor, TWOCCS("XN"), "(X)AND (N)OR", "Gate Logic"),
  public HSApplication {
public:
  OC_APP_INTERFACE_DECLARE(XandNor, XandNorDefs::kStorageSize);

  void Start();
  void Resume() {}

  void Controller() {
    using namespace XandNorDefs;
    bool gate[kNumChannels];
    for (int ch = 0; ch < kNumChannels; ++ch) gate[ch] = ProcessChannel(ch);

    for (int o = 0; o < kNumOutputs; ++o) {
      const Route &r = routes[o];
      const bool a = gate[r.a], b = gate[r.b];
      bool v;
      switch (r.fn) {
        case FN_NOT:  v = !a; break;
        case FN_AND:  v = a && b; break;
        case FN_NAND: v = !(a && b); break;
        case FN_OR:   v = a || b; break;
        case FN_NOR:  v = !(a || b); break;
        case FN_XOR:  v = a != b; break;
        case FN_XNOR: v = a == b; break;
        default:      v = a; break;
      }
      out_high[o] = v;
      GateOut(o, v);
    }
  }

  void View() const;

  void OnUpButtonPress();
  void OnUpButtonLongPress();
  void OnDownButtonPress();
  void ToggleRouting();
  void OnLeftEncoderMove(int direction);
  void OnRightEncoderMove(int direction);

private:
  // Settings (stored in presets from M3)
  DigitalInputMap source[XandNorDefs::kNumChannels];
  uint8_t length[XandNorDefs::kNumChannels];
  CVInputMap length_cv[XandNorDefs::kNumChannels];
  uint8_t cv_mode[XandNorDefs::kNumChannels];
  XandNorDefs::Route routes[8];
  XandNorDefs::PackedPatch reserved_store; // keeps unknown reserved bytes on resave

  // Engine state, owned by Controller()
  struct ChannelState {
    uint32_t elapsed; // ticks since the gate started
    uint32_t total;   // gate length in ticks, sampled at the trigger (Sample mode)
    uint8_t drop;     // ticks left in a retrigger drop
    bool active;      // the gate is running (high, or dropping before going high again)
  } state[XandNorDefs::kNumChannels];
  uint32_t live_us[XandNorDefs::kNumChannels]; // CV-modulated length, for display
  bool chan_high[XandNorDefs::kNumChannels];
  bool out_high[XandNorDefs::kNumOutputs];

  // UI
  int page = XandNorDefs::PAGE_OVERVIEW;
  int page_before_routing = XandNorDefs::PAGE_OVERVIEW; // for the X shortcut
  int cursor = 0;       // channel pages
  int route_cursor = 0; // routing page: output * ROUTE_FIELDS + field

  // Presets
  XandNorDefs::PackedPatch slots[XandNorDefs::kNumSlots];
  uint8_t slot_used = 0;   // bit per slot
  int active_slot = -1;    // last slot saved or loaded; -1 = none
  bool preset_mode = false;
  int slot_cursor = 0;
  mutable int message_frames = 0;
  const char *message = "";

  // Advances channel ch by one tick and returns its gate.
  bool ProcessChannel(int ch) {
    using namespace XandNorDefs;
    ChannelState &s = state[ch];
    const bool trig = source[ch].Clock(); // always called, to track edges
    const int len = length[ch];
    live_us[ch] = ModulatedLengthUs(ch);

    if (len == kLengthThru) {
      s.active = false;
      s.drop = 0;
      return chan_high[ch] = source[ch].Gate();
    }

    if (trig) {
      if (s.drop) {
        // already restarting
      } else if (s.active) {
        s.drop = kDropTicks;
      } else {
        StartGate(ch, len);
      }
    }

    if (s.drop) {
      if (--s.drop == 0) StartGate(ch, len);
      return chan_high[ch] = false;
    }

    if (s.active && len != kLengthInfinite) {
      const uint32_t total = (cv_mode[ch] == CV_TRACK) ? UsToTicks(live_us[ch]) : s.total;
      if (++s.elapsed >= total) s.active = false;
    }
    return chan_high[ch] = s.active;
  }

  void StartGate(int ch, int len) {
    state[ch].active = true;
    state[ch].elapsed = 0;
    state[ch].total = UsToTicks(live_us[ch]);
    (void)len;
  }

  static uint32_t UsToTicks(uint32_t us) {
    return (us + OC_CORE_TIMER_RATE / 2) / OC_CORE_TIMER_RATE;
  }

  // Channel length in microseconds with its length CV applied. Thru and
  // Infinite ignore CV; a finite length stays within 1 ms - 10 s, moving
  // smoothly between table steps (exponentially, by linear interpolation).
  uint32_t ModulatedLengthUs(int ch) const {
    using namespace XandNorDefs;
    const int len = length[ch];
    if (len == kLengthThru || len == kLengthInfinite) return 0;
    // position on the step scale in 1/256 steps
    int32_t pos = len * 256 + length_cv[ch].In() * kStepsPerVolt * 256 / ONE_OCTAVE;
    pos = constrain(pos, 256, kNumLengths * 256);
    const int idx = pos >> 8;
    if (idx >= kNumLengths) return kLengthUs[kNumLengths - 1];
    const uint32_t lo = kLengthUs[idx - 1], hi = kLengthUs[idx];
    return lo + ((hi - lo) * uint32_t(pos & 0xff) >> 8);
  }

  void DrawOverview() const;
  void DrawChannelPage(int ch) const;
  void DrawRouting() const;
  void DrawPageDots() const;
  void PrintLength(int setting) const;
  void PrintMicros(uint32_t us) const;
  void DrawFieldCursor(int x, int y, int w) const;
  void PrintRoute(int x, int y, const XandNorDefs::Route &r) const;
  void DrawPresets() const;

  void LoadDefaults();
  void PackPatch(XandNorDefs::PackedPatch &p) const;
  void UnpackPatch(const XandNorDefs::PackedPatch &p);
  void SaveSlot(int slot);
  void LoadSlot(int slot);
};

// -- Setup -------------------------------------------------------------------

FLASHMEM void XandNor::LoadDefaults() {
  using namespace XandNorDefs;
  // Unpack rather than Clear(): Clear() resets edge detection, so a high input
  // would fire every channel once.
  DigitalInputMap t1;
  t1.SetGateInput(0);
  for (int ch = 0; ch < kNumChannels; ++ch) {
    source[ch].Unpack(t1.Pack());
    length[ch] = kDefaultLengths[ch];
    length_cv[ch] = CVInputMap();
    cv_mode[ch] = CV_SAMPLE;
  }
  memset(&reserved_store, 0, sizeof(reserved_store));
  for (int o = 0; o < 8; ++o) routes[o] = kDefaultRoutes[o];
}

FLASHMEM void XandNor::Start() {
  using namespace XandNorDefs;
  LoadDefaults();
  for (int ch = 0; ch < kNumChannels; ++ch) {
    state[ch] = {0, 0, 0, false};
    live_us[ch] = 0;
    chan_high[ch] = false;
  }
  for (int o = 0; o < kNumOutputs; ++o) out_high[o] = false;
  slot_used = 0;
  active_slot = -1;
  page = PAGE_OVERVIEW;
  cursor = 0;
  route_cursor = 0;
}

// -- Presets -----------------------------------------------------------------

FLASHMEM void XandNor::PackPatch(XandNorDefs::PackedPatch &p) const {
  using namespace XandNorDefs;
  p = reserved_store; // carry reserved bytes through unchanged
  for (int ch = 0; ch < kNumChannels; ++ch) {
    p.source[ch] = source[ch].Pack();
    p.length[ch] = length[ch];
    p.extras[ch].length_cv = length_cv[ch].Pack();
    p.extras[ch].cv_mode = cv_mode[ch];
  }
  for (int o = 0; o < 8; ++o) p.routes[o] = routes[o];
}

// Validates as it goes, so a corrupt or foreign slot can't index out of range.
FLASHMEM void XandNor::UnpackPatch(const XandNorDefs::PackedPatch &p) {
  using namespace XandNorDefs;
  reserved_store = p;
  for (int ch = 0; ch < kNumChannels; ++ch) {
    source[ch].Unpack(p.source[ch]);
    length[ch] = constrain(p.length[ch], kLengthThru, kLengthInfinite);
    length_cv[ch].Unpack(p.extras[ch].length_cv);
    cv_mode[ch] = constrain(p.extras[ch].cv_mode, 0, CV_MODE_COUNT - 1);
  }
  for (int o = 0; o < 8; ++o) {
    Route r = p.routes[o];
    r.fn = constrain(r.fn, 0, FN_COUNT - 1);
    r.a = constrain(r.a, 0, kNumChannels - 1);
    r.b = constrain(r.b, 0, kNumChannels - 1);
    routes[o] = r;
  }
}

FLASHMEM void XandNor::SaveSlot(int slot) {
  PackPatch(slots[slot]);
  slot_used |= 1 << slot;
  active_slot = slot;
  message = "Saved";
  message_frames = XandNorDefs::kMessageFrames;
}

// An empty slot loads the default patch.
FLASHMEM void XandNor::LoadSlot(int slot) {
  if (slot_used & (1 << slot)) UnpackPatch(slots[slot]);
  else LoadDefaults();
  active_slot = slot;
  message = (slot_used & (1 << slot)) ? "Loaded" : "Defaults";
  message_frames = XandNorDefs::kMessageFrames;
}

// -- UI ----------------------------------------------------------------------

FLASHMEM void XandNor::View() const {
  using namespace XandNorDefs;
  if (preset_mode) {
    DrawPresets();
    return;
  }
  gfxHeader("(X)AND (N)OR");
  gfxPrint(128 - 6 * (int)strlen(kPageNames[page]) - 1, 1, kPageNames[page]);

  switch (page) {
    case PAGE_OVERVIEW: DrawOverview(); break;
    case PAGE_ROUTING:  DrawRouting(); break;
    default:            DrawChannelPage(page - PAGE_CHANNEL_A); break;
  }
  DrawPageDots();
}

FLASHMEM void XandNor::DrawOverview() const {
  using namespace XandNorDefs;
  // Channels A-D on the top row, outputs on the row below; filled = high.
  for (int ch = 0; ch < kNumChannels; ++ch) {
    const int x = 4 + ch * 31;
    gfxFrame(x, 16, 26, 12);
    gfxPrint(x + 4, 18, "CH");
    gfxPrint(ch + 1);
    if (chan_high[ch]) gfxInvert(x + 1, 17, 24, 10);
  }
  const int w = 128 / kNumOutputs;
  for (int o = 0; o < kNumOutputs; ++o) {
    const int x = o * w + (w - 12) / 2;
    gfxFrame(x, 34, 12, 12);
    const char name[2] = { char('A' + o), '\0' };
    gfxPrint(x + 3, 36, name);
    if (out_high[o]) gfxInvert(x + 1, 35, 10, 10);
  }
  if (active_slot >= 0) {
    gfxPrint(1, 48, "Preset ");
    gfxPrint(active_slot + 1);
  }
}

// A steady underline (the platform's gfxCursor blinks), so the selected field
// doesn't compete with output states flashing in time with the music.
FLASHMEM void XandNor::DrawFieldCursor(int x, int y, int w) const {
  gfxLine(x, y, x + w - 1, y);
  gfxPixel(x, y - 1);
  gfxPixel(x + w - 1, y - 1);
}

FLASHMEM void XandNor::PrintLength(int setting) const {
  using namespace XandNorDefs;
  if (setting == kLengthThru) { gfxPrint("Thru"); return; }
  if (setting == kLengthInfinite) { gfxPrint("Inf"); return; }
  PrintMicros(kLengthUs[setting - 1]);
}

FLASHMEM void XandNor::PrintMicros(uint32_t us) const {
  if (us < 10000) {        // 1.0 - 9.9 ms
    gfxPrint(int(us / 1000)); gfxPrint("."); gfxPrint(int(us / 100 % 10)); gfxPrint("ms");
  } else if (us < 1000000) {
    gfxPrint(int(us / 1000)); gfxPrint("ms");
  } else {                 // 1.0 - 10.0 s
    gfxPrint(int(us / 1000000)); gfxPrint("."); gfxPrint(int(us / 100000 % 10)); gfxPrint("s");
  }
}

FLASHMEM void XandNor::DrawChannelPage(int ch) const {
  using namespace XandNorDefs;
  // Five rows; the label column is 8 characters, values start at x=52.
  gfxPrint(1, 13, "Source:");
  gfxPrint(52, 13, source[ch].InputName());

  gfxPrint(1, 22, "Length:");
  gfxPos(52, 22);
  PrintLength(length[ch]);
  // With CV active, an arrow and the length it currently gives.
  if (length_cv[ch].enabled() && live_us[ch]) {
    gfxLine(86, 25, 90, 25);
    gfxLine(89, 24, 89, 26);
    gfxPos(94, 22);
    PrintMicros(live_us[ch]);
  }

  gfxPrint(1, 31, "Len CV:");
  gfxPrint(52, 31, length_cv[ch].InputName());

  gfxPrint(1, 40, "Depth:");
  const int tenths = Atten(length_cv[ch].attenuversion);
  gfxPos(52, 40);
  if (tenths < 0) gfxPrint("-");
  gfxPrint(abs(tenths) / 10);
  gfxPrint("%");

  gfxPrint(1, 49, "CV mode:");
  gfxPrint(52, 49, kCVModeNames[cv_mode[ch]]);

  // Live gate: a small box at the right of the header area.
  gfxFrame(121, 13, 6, 6);
  if (chan_high[ch]) gfxRect(122, 14, 4, 4);

  static constexpr int kRowY[FIELD_COUNT] = {13, 22, 31, 40, 49};
  DrawFieldCursor(51, kRowY[cursor] + 8, cursor == FIELD_LENGTH ? 31 : 37);
}

// Draws "FN a b" from x, e.g. "XOR 3 4" for CH3 XOR CH4. Only channel numbers
// fit in half the screen; the function gets a 4-character column so the
// operands line up down the page.
FLASHMEM void XandNor::PrintRoute(int x, int y, const XandNorDefs::Route &r) const {
  using namespace XandNorDefs;
  gfxPrint(x, y, kFunctionNames[r.fn]);
  gfxPrint(x + 28, y, r.a + 1);
  if (TwoInputs(r.fn)) gfxPrint(x + 40, y, r.b + 1);
}

FLASHMEM void XandNor::DrawRouting() const {
  using namespace XandNorDefs;
  // Two columns of four outputs under a "Fn / Ch" header; the output letter is
  // inverted while the output is high.
  for (int col = 0; col * 4 < kNumOutputs; ++col) {
    const int x = col * 64 + 1;
    gfxPrint(x + 8, 13, "Fn");
    gfxPrint(x + 36, 13, "Ch");
    gfxDottedLine(x, 20, x + 55, 20);
  }
  for (int o = 0; o < kNumOutputs; ++o) {
    const int x = (o / 4) * 64 + 1;
    const int y = 22 + (o % 4) * 9;
    const char name[2] = { char('A' + o), '\0' };
    gfxPrint(x, y, name);
    if (out_high[o]) gfxInvert(x - 1, y - 1, 7, 9);
    PrintRoute(x + 8, y, routes[o]);
  }

  // Underline the selected field: function, first or second channel.
  const int o = route_cursor / ROUTE_FIELDS;
  const int x = (o / 4) * 64 + 1;
  const int y = 22 + (o % 4) * 9 + 8;
  switch (route_cursor % ROUTE_FIELDS) {
    case ROUTE_FN: DrawFieldCursor(x + 8, y, 25); break;
    case ROUTE_A:  DrawFieldCursor(x + 36, y, 6); break;
    default:       DrawFieldCursor(x + 48, y, 6); break;
  }
}

// Preset overlay: slots 1-8 in two rows. The selected slot is inverted, used
// slots carry a dot, and the active slot is framed twice.
FLASHMEM void XandNor::DrawPresets() const {
  using namespace XandNorDefs;
  gfxHeader("(X)AND (N)OR");
  gfxPrint(128 - 6 * 7 - 1, 1, "Presets");
  for (int i = 0; i < kNumSlots; ++i) {
    const int x = 4 + (i % 4) * 31;
    const int y = 15 + (i / 4) * 17;
    gfxFrame(x, y, 26, 14);
    if (i == active_slot) gfxFrame(x + 1, y + 1, 24, 12);
    gfxPrint(x + 6, y + 3, i + 1);
    if (slot_used & (1 << i)) gfxRect(x + 16, y + 5, 3, 3);
    if (i == slot_cursor) gfxInvert(x, y, 26, 14);
  }
  if (message_frames > 0) {
    --message_frames;
    gfxPrint(64 - 3 * (int)strlen(message), 52, message);
  } else {
    gfxPrint(1, 52, kPresetHint);
  }
}

FLASHMEM void XandNor::DrawPageDots() const {
  using namespace XandNorDefs;
  const int x0 = 64 - PAGE_COUNT * 4;
  for (int p = 0; p < PAGE_COUNT; ++p) {
    if (p == page) gfxRect(x0 + p * 8, 58, 5, 5);
    else gfxFrame(x0 + p * 8, 58, 5, 5);
  }
}

FLASHMEM void XandNor::OnUpButtonPress() {
  if (preset_mode) { SaveSlot(slot_cursor); return; }
  page = (page + XandNorDefs::PAGE_COUNT - 1) % XandNorDefs::PAGE_COUNT;
}

FLASHMEM void XandNor::OnUpButtonLongPress() {
  preset_mode = !preset_mode;
  if (preset_mode) {
    slot_cursor = active_slot >= 0 ? active_slot : 0;
    message_frames = 0;
  }
}

// X shortcut (Teensy 4.1): jump to Routing, or back to where you came from.
FLASHMEM void XandNor::ToggleRouting() {
  using namespace XandNorDefs;
  preset_mode = false;
  if (page == PAGE_ROUTING) {
    page = page_before_routing;
  } else {
    page_before_routing = page;
    page = PAGE_ROUTING;
  }
}

FLASHMEM void XandNor::OnDownButtonPress() {
  if (preset_mode) { LoadSlot(slot_cursor); return; }
  page = (page + 1) % XandNorDefs::PAGE_COUNT;
}

FLASHMEM void XandNor::OnLeftEncoderMove(int direction) {
  using namespace XandNorDefs;
  if (preset_mode) {
    slot_cursor = constrain(slot_cursor + direction, 0, kNumSlots - 1);
    return;
  }
  if (page == PAGE_ROUTING) {
    // Step through fields, skipping the second channel of OUT and NOT.
    const int last = kNumOutputs * ROUTE_FIELDS - 1;
    int c = route_cursor;
    do {
      c += (direction > 0) ? 1 : -1;
    } while (c >= 0 && c <= last && c % ROUTE_FIELDS == ROUTE_B &&
             !TwoInputs(routes[c / ROUTE_FIELDS].fn));
    if (c >= 0 && c <= last) route_cursor = c;
    return;
  }
  if (page < PAGE_CHANNEL_A || page > PAGE_CHANNEL_D) return;
  cursor = constrain(cursor + direction, 0, FIELD_COUNT - 1);
}

FLASHMEM void XandNor::OnRightEncoderMove(int direction) {
  using namespace XandNorDefs;
  if (preset_mode) {
    slot_cursor = constrain(slot_cursor + direction, 0, kNumSlots - 1);
    return;
  }
  if (page == PAGE_ROUTING) {
    Route &r = routes[route_cursor / ROUTE_FIELDS];
    switch (route_cursor % ROUTE_FIELDS) {
      case ROUTE_FN:
        r.fn = constrain(r.fn + direction, 0, FN_COUNT - 1);
        break;
      case ROUTE_A:
        r.a = constrain(r.a + direction, 0, kNumChannels - 1);
        break;
      default:
        r.b = constrain(r.b + direction, 0, kNumChannels - 1);
        break;
    }
    return;
  }
  if (page < PAGE_CHANNEL_A || page > PAGE_CHANNEL_D) return;
  const int ch = page - PAGE_CHANNEL_A;
  switch (cursor) {
    case FIELD_SOURCE:
      source[ch].ChangeSource(direction);
      break;
    case FIELD_LENGTH:
      length[ch] = constrain(length[ch] + direction, kLengthThru, kLengthInfinite);
      break;
    case FIELD_CV:
      length_cv[ch].ChangeSource(direction);
      break;
    case FIELD_DEPTH:
      length_cv[ch].attenuversion = constrain(length_cv[ch].attenuversion + direction, -127, 127);
      break;
    default:
      cv_mode[ch] = constrain(cv_mode[ch] + direction, 0, CV_MODE_COUNT - 1);
      break;
  }
}

// -- App interface ------------------------------------------------------------

FLASHMEM void XandNor::Init() { BaseStart(); }

void XandNor::Process(OC::IOFrame *ioframe) { BaseController(ioframe); }

FLASHMEM void XandNor::GetIOConfig(OC::IOConfig &ioconfig) const {
  using namespace OC;
  ioconfig.digital_inputs[DIGITAL_INPUT_1].set("Trig");
  ioconfig.digital_inputs[DIGITAL_INPUT_2].set("");
  ioconfig.digital_inputs[DIGITAL_INPUT_3].set("");
  ioconfig.digital_inputs[DIGITAL_INPUT_4].set("");
  for (int o = 0; o < XandNorDefs::kNumOutputs; ++o)
    ioconfig.outputs[o].set("Gate", OUTPUT_MODE_GATE);
}

FLASHMEM size_t XandNor::SaveAppData(util::StreamBufferWriter &stream_buffer) const {
  using namespace XandNorDefs;
  PackedPatch working;
  PackPatch(working);
  stream_buffer.Write(kStorageVersion);
  stream_buffer.Write(slot_used);
  stream_buffer.Write(working);
  for (int i = 0; i < kNumSlots; ++i) stream_buffer.Write(slots[i]);
  return stream_buffer.written();
}

FLASHMEM size_t XandNor::RestoreAppData(util::StreamBufferReader &stream_buffer) {
  using namespace XandNorDefs;
  if (stream_buffer.Read<uint8_t>() != kStorageVersion) return stream_buffer.read();
  const uint8_t used = stream_buffer.Read<uint8_t>();
  PackedPatch working, stored[kNumSlots];
  stream_buffer.Read(working);
  for (int i = 0; i < kNumSlots; ++i) stream_buffer.Read(stored[i]);
  if (stream_buffer.underflow()) return stream_buffer.read();

  slot_used = used;
  for (int i = 0; i < kNumSlots; ++i) slots[i] = stored[i];
  UnpackPatch(working);
  return stream_buffer.read();
}

FLASHMEM void XandNor::HandleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Resume();
}

void XandNor::Loop() {}

FLASHMEM void XandNor::DrawMenu() const { BaseView(); }

FLASHMEM void XandNor::DrawScreensaver() const { BaseScreensaver(); }

void XandNor::DrawDebugInfo() const {}

FLASHMEM void XandNor::HandleButtonEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_BUTTON_UP) {
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnUpButtonLongPress();
    if (event.type == UI::EVENT_BUTTON_PRESS) OnUpButtonPress();
  }
  if (event.control == OC::CONTROL_BUTTON_DOWN && event.type == UI::EVENT_BUTTON_PRESS)
    OnDownButtonPress();
#ifdef ARDUINO_TEENSY41
  // Extra ORN8 buttons: X toggles Routing, Y toggles the preset overlay.
  // Z is left alone: the platform uses it as a modifier for its menus.
  if (event.type == UI::EVENT_BUTTON_PRESS) {
    if (event.control == OC::CONTROL_BUTTON_X) ToggleRouting();
    if (event.control == OC::CONTROL_BUTTON_Y) OnUpButtonLongPress();
  }
#endif
  // Encoder press closes the preset overlay.
  if (preset_mode && event.type == UI::EVENT_BUTTON_PRESS &&
      (event.control == OC::CONTROL_BUTTON_L || event.control == OC::CONTROL_BUTTON_R))
    preset_mode = false;
}

FLASHMEM void XandNor::HandleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoderMove(event.value);
}
