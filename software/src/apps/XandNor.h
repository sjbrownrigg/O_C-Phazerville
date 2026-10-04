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
// (X)AND (N)OR - four gate-length processors (channels A-D) feeding freely
// routed gate outputs. Each output carries a channel's gate, its inverse, or a
// logic function (AND, NAND, OR, NOR, XOR, XNOR) of two channels.
//
// All outputs are used: 8 on Teensy 4.1 builds, 4 on Teensy 4.0.
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
  "Overview", "Channel A", "Channel B", "Channel C", "Channel D", "Routing"
};

} // namespace XandNorDefs

OC_APP_CLASS(XandNor, TWOCCS("XN"), "(X)AND (N)OR", "Gate Logic"),
  public HSApplication {
public:
  OC_APP_INTERFACE_DECLARE(XandNor, 0);

  void Start();
  void Resume() {}

  void Controller() {
  }

  void View() const;

  void OnUpButtonPress();
  void OnDownButtonPress();

private:
  int page = XandNorDefs::PAGE_OVERVIEW;

  void DrawOverview() const;
  void DrawChannelPage(int ch) const;
  void DrawRouting() const;
  void DrawPageDots() const;
};

// -- UI ----------------------------------------------------------------------

FLASHMEM void XandNor::Start() {
  page = XandNorDefs::PAGE_OVERVIEW;
}

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
  // Channels A-D on the top row, outputs on the row below.
  for (int ch = 0; ch < kNumChannels; ++ch) {
    const int x = 4 + ch * 31;
    gfxFrame(x, 16, 26, 12);
    const char name[2] = { char('A' + ch), '\0' };
    gfxPrint(x + 10, 18, name);
  }
  const int w = 128 / kNumOutputs;
  for (int o = 0; o < kNumOutputs; ++o) {
    const int x = o * w + (w - 12) / 2;
    gfxFrame(x, 34, 12, 12);
    gfxPrint(x + 3, 36, o + 1);
  }
}

FLASHMEM void XandNor::DrawChannelPage(int ch) const {
  gfxPrint(1, 18, "Source:");
  gfxPrint(1, 30, "Length:");
  (void)ch; // channel settings arrive in M1
}

FLASHMEM void XandNor::DrawRouting() const {
  using namespace XandNorDefs;
  // Two columns of four outputs.
  for (int o = 0; o < kNumOutputs; ++o) {
    const int x = (o / 4) * 64 + 1;
    const int y = 14 + (o % 4) * 10;
    gfxPrint(x, y, o + 1);
    gfxPrint(":");
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

FLASHMEM void XandNor::HandleEncoderEvent(const UI::Event &) {}
