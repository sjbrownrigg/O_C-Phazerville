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
// All outputs are used: A-H on Teensy 4.1 builds, A-D on Teensy 4.0, matching
// the panel labels.
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

static constexpr uint32_t LengthTicks(int setting) {
  return (kLengthUs[setting - 1] + OC_CORE_TIMER_RATE / 2) / OC_CORE_TIMER_RATE;
}

enum ChannelField { FIELD_SOURCE, FIELD_LENGTH, FIELD_COUNT };

// Routing page cursor: each output has three fields, the second channel only
// for two-input functions.
enum RouteField { ROUTE_FN, ROUTE_A, ROUTE_B, ROUTE_FIELDS };

static constexpr bool TwoInputs(uint8_t fn) { return fn != FN_OUT && fn != FN_NOT; }

} // namespace XandNorDefs

OC_APP_CLASS(XandNor, TWOCCS("XN"), "(X)AND (N)OR", "Gate Logic"),
  public HSApplication {
public:
  OC_APP_INTERFACE_DECLARE(XandNor, 0);

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
  void OnDownButtonPress();
  void OnLeftEncoderMove(int direction);
  void OnRightEncoderMove(int direction);

private:
  // Settings (stored in presets from M3)
  DigitalInputMap source[XandNorDefs::kNumChannels];
  uint8_t length[XandNorDefs::kNumChannels];
  XandNorDefs::Route routes[8];

  // Engine state, owned by Controller()
  struct ChannelState {
    uint32_t remaining; // ticks left in a finite gate
    uint8_t drop;       // ticks left in a retrigger drop
    bool active;        // the gate is running (high, or dropping before going high again)
  } state[XandNorDefs::kNumChannels];
  bool chan_high[XandNorDefs::kNumChannels];
  bool out_high[XandNorDefs::kNumOutputs];

  // UI
  int page = XandNorDefs::PAGE_OVERVIEW;
  int cursor = 0;       // channel pages
  int route_cursor = 0; // routing page: output * ROUTE_FIELDS + field

  // Advances channel ch by one tick and returns its gate.
  bool ProcessChannel(int ch) {
    using namespace XandNorDefs;
    ChannelState &s = state[ch];
    const bool trig = source[ch].Clock(); // always called, to track edges
    const int len = length[ch];

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
      if (s.remaining == 0 || --s.remaining == 0) s.active = false;
    }
    return chan_high[ch] = s.active;
  }

  void StartGate(int ch, int len) {
    using namespace XandNorDefs;
    state[ch].active = true;
    state[ch].remaining = (len == kLengthInfinite) ? 0 : LengthTicks(len);
  }

  void DrawOverview() const;
  void DrawChannelPage(int ch) const;
  void DrawRouting() const;
  void DrawPageDots() const;
  void PrintLength(int setting) const;
  void DrawFieldCursor(int x, int y, int w) const;
  void PrintRoute(int x, int y, const XandNorDefs::Route &r) const;
};

// -- Setup -------------------------------------------------------------------

FLASHMEM void XandNor::Start() {
  using namespace XandNorDefs;
  for (int ch = 0; ch < kNumChannels; ++ch) {
    source[ch].SetGateInput(0);
    length[ch] = kDefaultLengths[ch];
    state[ch] = {0, 0, false};
    chan_high[ch] = false;
  }
  for (int o = 0; o < 8; ++o) routes[o] = kDefaultRoutes[o];
  for (int o = 0; o < kNumOutputs; ++o) out_high[o] = false;
  page = PAGE_OVERVIEW;
  cursor = 0;
  route_cursor = 0;
}

// -- UI ----------------------------------------------------------------------

FLASHMEM void XandNor::View() const {
  using namespace XandNorDefs;
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
  const uint32_t us = kLengthUs[setting - 1];
  if (us < 10000) {        // 1.0 - 8.0 ms
    gfxPrint(int(us / 1000)); gfxPrint("."); gfxPrint(int(us / 100 % 10)); gfxPrint("ms");
  } else if (us < 1000000) {
    gfxPrint(int(us / 1000)); gfxPrint("ms");
  } else {                 // 1.0 - 10.0 s
    gfxPrint(int(us / 1000000)); gfxPrint("."); gfxPrint(int(us / 100000 % 10)); gfxPrint("s");
  }
}

FLASHMEM void XandNor::DrawChannelPage(int ch) const {
  using namespace XandNorDefs;
  gfxPrint(1, 16, "Source:");
  gfxPrint(52, 16, source[ch].InputName());
  gfxPrint(1, 28, "Length:");
  gfxPos(52, 28);
  PrintLength(length[ch]);
  DrawFieldCursor(51, cursor == FIELD_SOURCE ? 24 : 36, 48);

  // Live gate indicator
  gfxFrame(1, 42, 126, 8);
  if (chan_high[ch]) gfxRect(2, 43, 124, 6);
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

FLASHMEM void XandNor::DrawPageDots() const {
  using namespace XandNorDefs;
  const int x0 = 64 - PAGE_COUNT * 4;
  for (int p = 0; p < PAGE_COUNT; ++p) {
    if (p == page) gfxRect(x0 + p * 8, 58, 5, 5);
    else gfxFrame(x0 + p * 8, 58, 5, 5);
  }
}

FLASHMEM void XandNor::OnUpButtonPress() {
  page = (page + XandNorDefs::PAGE_COUNT - 1) % XandNorDefs::PAGE_COUNT;
}

FLASHMEM void XandNor::OnDownButtonPress() {
  page = (page + 1) % XandNorDefs::PAGE_COUNT;
}

FLASHMEM void XandNor::OnLeftEncoderMove(int direction) {
  using namespace XandNorDefs;
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
  if (cursor == FIELD_SOURCE) {
    source[ch].ChangeSource(direction);
  } else {
    length[ch] = constrain(length[ch] + direction, kLengthThru, kLengthInfinite);
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

FLASHMEM size_t XandNor::SaveAppData(util::StreamBufferWriter &) const { return 0; }
FLASHMEM size_t XandNor::RestoreAppData(util::StreamBufferReader &) { return 0; }

FLASHMEM void XandNor::HandleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Resume();
}

void XandNor::Loop() {}

FLASHMEM void XandNor::DrawMenu() const { BaseView(); }

FLASHMEM void XandNor::DrawScreensaver() const { BaseScreensaver(); }

void XandNor::DrawDebugInfo() const {}

FLASHMEM void XandNor::HandleButtonEvent(const UI::Event &event) {
  if (event.type != UI::EVENT_BUTTON_PRESS) return;
  if (event.control == OC::CONTROL_BUTTON_UP) OnUpButtonPress();
  if (event.control == OC::CONTROL_BUTTON_DOWN) OnDownButtonPress();
}

FLASHMEM void XandNor::HandleEncoderEvent(const UI::Event &event) {
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoderMove(event.value);
}
