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
// (X)AND(N)OR - four gate-length processors (channels CH1-CH4) feeding freely
// routed gate outputs. Each output carries a channel's gate, its inverse, or a
// logic function (AND, NAND, OR, NOR, XOR, XNOR) of two channels.
//
// Built for the Teensy 4.1 (ORN8) only: outputs A-H, CV inputs 1-8, matching
// the panel labels. (The Teensy 4.0 build has no room for its preset area.)
//
// Only Controller() runs in the 16.7 kHz ISR. Everything else is defined out of
// class and marked FLASHMEM: in-class definitions stay in RAM1 regardless.

#pragma once

// Audio applets (M8): the platform's audio engine, `audio_app`, exists only on
// Teensy 4.1 builds with Hemisphere; Quadrants hosts it too, and both apps
// share the same audio setup.
#if defined(ARDUINO_TEENSY41) && !defined(NO_HEMISPHERE)
#define XANDNOR_AUDIO 1
#endif

namespace XandNorDefs {

enum Page {
  PAGE_OVERVIEW,
  PAGE_CHANNEL_A,
  PAGE_CHANNEL_B,
  PAGE_CHANNEL_C,
  PAGE_CHANNEL_D,
  PAGE_ROUTING,
  PAGE_OUTPUTS,
  PAGE_CVMOD,
  PAGE_AUDIO,
  PAGE_SCOPE,
  PAGE_COUNT
};

static constexpr int kNumChannels = 4;
static constexpr int kNumOutputs = DAC_CHANNEL_COUNT;

static const char * const kPageNames[PAGE_COUNT] = {
  "Overview", "CH1", "CH2", "CH3", "CH4", "Routing", "Outputs", "CV Mod", "Audio", "Scope"
};

// Output functions. OUT and NOT read one channel; the rest combine two.
// The first eight are logic; the rest (M7) keep state or output CV:
//   TOGL flips on each rising edge of a; LTCH is set by a and reset by b;
//   CNT and BIN read all four channels as a stepped CV (count, or CH1-CH4 as
//   bits); ENV is an AD or ASR envelope on channel a.
enum Function {
  FN_OUT, FN_NOT, FN_AND, FN_NAND, FN_OR, FN_NOR, FN_XOR, FN_XNOR,
  FN_TOGL, FN_LTCH, FN_CNT, FN_BIN, FN_ENV,
  FN_COUNT
};
static constexpr int kNumLogicFns = FN_TOGL; // function-by-CV stays within these
static const char * const kFunctionNames[FN_COUNT] = {
  "OUT", "NOT", "AND", "NAND", "OR", "NOR", "XOR", "XNOR",
  "TOGL", "LTCH", "CNT", "BIN", "ENV"
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

// Channel page: a scrolling list of rows, some with two fields (a CV source
// and its depth, or the trigger source and its divider).
enum ChannelField {
  FIELD_SOURCE, FIELD_DIV,
  FIELD_LENGTH,
  FIELD_LEN_CV, FIELD_LEN_DEPTH,
  FIELD_LEN_MODE,
  FIELD_DELAY,
  FIELD_DELAY_CV, FIELD_DELAY_DEPTH,
  FIELD_PROB,
  FIELD_PROB_CV, FIELD_PROB_DEPTH,
  FIELD_RATCHET,
  FIELD_RAT_CV, FIELD_RAT_DEPTH,
  FIELD_DIV_CV, FIELD_DIV_DEPTH,
  FIELD_MODE,
  FIELD_COUNT
};
struct FieldPos { uint8_t row, x, w; };
static constexpr FieldPos kFieldPos[FIELD_COUNT] = {
  {0, 52, 18}, {0, 76, 18},
  {1, 52, 31},
  {2, 52, 18}, {2, 88, 30},
  {3, 52, 30},
  {4, 52, 31},
  {5, 52, 18}, {5, 88, 30},
  {6, 52, 24},
  {7, 52, 18}, {7, 88, 30},
  {8, 52, 12},
  {9, 52, 18}, {9, 88, 30},
  {10, 52, 18}, {10, 88, 30},
  {11, 52, 37},
};
static constexpr int kChannelRows = 12;

// CV Mod page: outputs A-H (function CV source and depth, with the function in
// use), then the preset CV and its load trigger.
enum ModField {
  MOD_FN_CV,          // + 2 * output: source; + 1: depth
  MOD_PRESET_CV = 16, MOD_PRESET_DEPTH, MOD_PRESET_TRIG,
  MOD_PRESET_AUDIO, // whether preset loads also load their audio setup
  MOD_COUNT
};
static constexpr int kModRows = 11;
static constexpr FieldPos ModFieldPos(int f) {
  return f < MOD_PRESET_CV ? FieldPos{uint8_t(f / 2), uint8_t(f % 2 ? 40 : 14), uint8_t(f % 2 ? 30 : 18)}
       : f == MOD_PRESET_CV ? FieldPos{8, 52, 18}
       : f == MOD_PRESET_DEPTH ? FieldPos{8, 88, 30}
       : f == MOD_PRESET_TRIG ? FieldPos{9, 52, 18}
       : FieldPos{10, 52, 18};
}
static constexpr int kVisibleRows = 5;
static const char * const kRowLabels[kChannelRows] = {
  "Source:", "Length:", "  CV:", "Len by:", "Delay:", "  CV:",
  "Prob:", "  CV:", "Ratchet:", "  CV:", "Div CV:", "CV mode:"
};

// Delay: 0 = off, else a step on the length scale. Up to kDelayQueue triggers
// can wait at once, so a delay longer than the clock period still works.
static constexpr int kDelayQueue = 4;
// Clock-relative length: a percentage of the measured trigger period. Periods
// over 10 s count as no tempo, and the time length applies instead.
static constexpr uint8_t kDefaultClockPct = 50;
static constexpr uint32_t kMaxPeriodTicks = 10 * 16666;
static constexpr uint32_t kMinGateTicks = 17; // 1 ms

// Ratchet: up to 8 pulses per gate, each high for at least 1 ms (17 ticks).
static constexpr int kMaxRatchets = 8;
static constexpr uint32_t kMinRatchetPeriod = 34;
// CV scaling at 100% depth: +5 V gives +100% probability, +7 ratchets, and
// +10 divider steps (2 per volt).
static constexpr int kDivStepsPerVolt = 2;

// The divider runs on one scale, ... /3, /2, /1, x2, x3 ...: index 0 is /1,
// positive indices multiply and negative divide, so turning right or raising
// the CV speeds the channel up, as on PAMs. The platform's steps value is the
// other way round (positive divides) and has no 0, which would silence the
// channel, so convert at the edges.
static constexpr int kDivIndexMin = 1 - CLOCKDIV_MAX;  // /64
static constexpr int kDivIndexMax = -1 - CLOCKDIV_MIN; // x24
static constexpr int DivIndexToSteps(int k) { return k <= 0 ? 1 - k : -(k + 1); }
static constexpr int DivStepsToIndex(int s) { return s > 0 ? 1 - s : -s - 1; }
static_assert(DivIndexToSteps(0) == 1 && DivIndexToSteps(-1) == 2 && DivIndexToSteps(1) == -2 &&
              DivIndexToSteps(kDivIndexMin) == CLOCKDIV_MAX && DivIndexToSteps(kDivIndexMax) == CLOCKDIV_MIN &&
              DivStepsToIndex(DivIndexToSteps(-5)) == -5 && DivStepsToIndex(DivIndexToSteps(7)) == 7,
              "divider index mapping");

// How CV modulation is read: once per trigger, or continuously.
enum CVMode : uint8_t { CV_SAMPLE, CV_TRACK, CV_MODE_COUNT };
static const char * const kCVModeNames[CV_MODE_COUNT] = {"Sample", "Track"};

// Length CV moves the length 8 steps per volt on the 10-steps-per-decade scale,
// so +5 V at 100% depth spans the whole 1 ms - 10 s range.
static constexpr int kStepsPerVolt = 8;

// Routing page cursor: each output has three fields, the second channel only
// for two-input functions.
enum RouteField { ROUTE_FN, ROUTE_A, ROUTE_B, ROUTE_FIELDS };

static constexpr bool TwoInputs(uint8_t fn) {
  return (fn >= FN_AND && fn <= FN_XNOR) || fn == FN_LTCH;
}
static constexpr bool UsesA(uint8_t fn) { return fn != FN_CNT && fn != FN_BIN; }

// Per-output settings for the M7 functions (Outputs page).
enum CVStep : uint8_t { CVSTEP_SEMI, CVSTEP_THIRD, CVSTEP_VOLT, CVSTEP_COUNT };
static const char * const kStepNames[CVSTEP_COUNT] = {"Semi", "1/3V", "1V"};
static constexpr int kStepCV[CVSTEP_COUNT] = {ONE_OCTAVE / 12, ONE_OCTAVE / 3, ONE_OCTAVE};
enum EnvMode : uint8_t { EMODE_AD, EMODE_ASR, EMODE_COUNT };
static const char * const kEnvModeNames[EMODE_COUNT] = {"AD", "ASR"};
static constexpr uint8_t kDefaultAttack = 4;  // 2 ms, a step on the length scale
static constexpr uint8_t kDefaultDecay = 24;  // 200 ms
static constexpr int32_t kEnvMax = 5 * ONE_OCTAVE * 256; // 5 V, 8 fractional bits

// A patch as stored in EEPROM and in the preset slots.
// Per-channel settings beyond source and length. The reserved bytes hold
// M5/M6 settings later without changing the stored size, which would make
// the platform discard saved data.
// Stored so that all-zero bytes (as saved before these fields existed) mean
// the defaults: probability 100%, one ratchet, CV inputs off.
struct ChannelExtras {
  uint16_t length_cv; // CVInputMap::Pack(); 0 = default
  uint8_t cv_mode;
  uint8_t prob_skip;  // 100 - probability
  uint16_t prob_cv;
  uint8_t ratchet_extra; // ratchets - 1
  uint8_t delay;         // 0 = off
  uint16_t ratchet_cv;
  uint16_t div_cv;
  uint16_t delay_cv;
  uint8_t clock_pct;     // 0 = default (kDefaultClockPct)
  uint8_t flags;         // bit 0: clock-relative length; bit 1: level mode;
                         // bits 2-7: level threshold in 0.1 V, 0 = default
};
static constexpr uint8_t kFlagClockLength = 1;
static constexpr uint8_t kFlagLevel = 2;
// Level mode: the channel is high while its source is above a threshold, with
// 0.2 V of hysteresis, so an LFO becomes a gate whose width follows its shape.
static constexpr uint8_t kDefaultThresholdDv = 12; // 1.2 V, near the platform's 1.25 V
static constexpr uint8_t kMaxThresholdDv = 63;
static constexpr int kHysteresis = ONE_OCTAVE / 10; // 0.1 V each side
static_assert(sizeof(ChannelExtras) == 16, "ChannelExtras layout changed");

struct PackedPatch {
  uint32_t source[kNumChannels]; // DigitalInputMap::Pack()
  uint8_t length[kNumChannels];
  Route routes[8];
  ChannelExtras extras[kNumChannels];
  // Patch-wide settings, in what was the 20-byte reserve.
  uint16_t fn_cv[8];   // per output: CV moves the logic function
  uint16_t preset_cv;  // CV picks the preset slot to load (global, see below)
  uint8_t preset_trig; // DigitalInputMap source byte: the trigger that loads it
  uint8_t reserved;
  // Storage v3: room for M7/M8 settings. Zero means "not set" throughout.
  struct OutputExtras {
    uint8_t step;     // CVStep for CNT/BIN
    uint8_t env_mode; // EnvMode
    uint8_t attack;   // length-scale step; 0 = default
    uint8_t decay;    // length-scale step; 0 = default
    uint8_t reserved[4];
  } out[8];
  uint8_t ext_reserved[64]; // M8
};
static_assert(sizeof(PackedPatch) == 256, "PackedPatch layout changed");
static constexpr size_t kPatchV2Size = 128; // the patch before ext existed

static constexpr int kNumSlots = 8;
// v3: version, current bank, working patch; the slots live in bank files.
// v2 (read once, to migrate): version, slot-used mask, working patch and
// eight slots, all 128-byte patches.
static constexpr uint8_t kStorageVersion = 3;
static constexpr uint8_t kStorageVersionV2 = 2;
// Fixed at the v2 size: the platform discards saved data whose size changes,
// so keeping it lets v2 data reach RestoreAppData() and migrate.
static constexpr size_t kStorageSize = 2 + 9 * kPatchV2Size;
static_assert(2 + sizeof(PackedPatch) <= kStorageSize, "v3 data must fit the fixed size");

// Preset banks: 8 slots per file on the SD card, else internal flash.
static constexpr int kNumBanks = 100;
struct BankHeader {
  char magic[4];     // "XNBK"
  uint8_t version;   // kBankVersion
  uint8_t slot_used; // bit per slot
  uint8_t pad[2];
};
static constexpr uint8_t kBankVersion = 1;
static constexpr uint32_t kMessageMs = 2500; // how long "Saved" etc. stay up

// Scope: a logic-analyser view of the app's own signals. Each column holds
// one bit per signal (CH1-CH4 in bits 0-3, outputs A-H in bits 4-11), OR-ed
// over the column's time so short pulses still show at slow timebases.
static constexpr int kScopeCols = 114;   // x = 14..127, after the labels
static constexpr int kScopeTraces = 8;
// Trace signals: 0 off, 1-4 CH1-CH4, 5-12 outputs A-H (on/off), 13-20 CV
// inputs 1-8 (voltage, labelled V1-V8 so as not to clash with C1-C4).
static constexpr int kScopeCVInputs = 8;
static constexpr int kScopeFirstCV = 1 + kNumChannels + 8;
static constexpr int kScopeSignals = kScopeFirstCV + kScopeCVInputs;
// CV traces store a column's lowest and highest voltage as int8, 72 CV units
// (about 47 mV) per step; drawn over -5..+5 V across the trace's height.
static constexpr int kScopeCVScale = 72;
static constexpr int kScopeCVRange = 5 * ONE_OCTAVE / kScopeCVScale; // 5 V
static constexpr uint8_t kDefaultTraces[kScopeTraces] = {1, 2, 3, 4, 9, 10, 11, 12}; // C1-C4, E-H
// Timebase: 0 = Auto (two trigger periods per screen), else ms per screen.
static constexpr uint16_t kScopeSpanMs[] = {
  0, 5, 10, 15, 20, 30, 50, 75, 100, 150, 200, 300, 500, 750,
  1000, 1500, 2000, 3000, 5000, 7500, 10000};
static constexpr int kScopeSpans = sizeof(kScopeSpanMs) / sizeof(kScopeSpanMs[0]);
static constexpr uint16_t kScopeFreeSpanMs = 1000; // Auto with no tempo yet
// Roll + Auto: a fixed span, so the scroll speed never depends on the clock
// (2 s is about one column per display frame).
static constexpr uint16_t kRollAutoSpanMs = 2000;
// Roll scrolls the picture; Sweep redraws it in place as new columns arrive;
// Trig captures one sweep per trigger. Roll and Sweep capture continuously.
enum ScopeMode : uint8_t { SCOPE_ROLL, SCOPE_SWEEP, SCOPE_TRIGGERED, SCOPE_MODES };
static const char * const kScopeModeNames[SCOPE_MODES] = {"Roll", "Swp", "Trig"};
static constexpr int kSweepGap = 0; // blank columns ahead of the write point (0 = none)
// Capture: Peak ORs a column's states (no pulse missed) and keeps a CV's range;
// Sample takes the state at the column's end, as the Scope applet does.
enum ScopeField {
  SCOPE_MODE, SCOPE_TRIG, SCOPE_SPAN, SCOPE_CAPTURE,
  SCOPE_TRACE0, SCOPE_FIELDS = SCOPE_TRACE0 + kScopeTraces
};

// Up/Down are labelled A/B on the Teensy 4.1 (ORN8) panel.
#ifdef ARDUINO_TEENSY41
static const char * const kPresetHint = "A:Save  B:Load";
#else
static const char * const kPresetHint = "Up:Save  Down:Load";
#endif

} // namespace XandNorDefs

OC_APP_CLASS(XandNor, TWOCCS("XN"), "(X)AND(N)OR", "Gate Logic"),
  public HSApplication {
public:
  OC_APP_INTERFACE_DECLARE(XandNor, XandNorDefs::kStorageSize);

  void Start();
  void Resume() {}

  void Controller() {
    using namespace XandNorDefs;
    bool gate[kNumChannels];
    for (int ch = 0; ch < kNumChannels; ++ch) gate[ch] = ProcessChannel(ch);

    // Preset by CV: the trigger picks a slot (by CV, or the next used slot
    // with no CV); Loop() loads it outside the interrupt.
    if (preset_trig.Clock()) {
      pending_slot = preset_cv.enabled()
        ? constrain(preset_cv.In() * kNumSlots / (5 * ONE_OCTAVE), 0, kNumSlots - 1)
        : -2;
    }

    // The clock manager sets auto_reset when its clock starts, and every
    // DigitalInputMap::Clock() resets its edge detection while it's set. The
    // hosting app must clear it after one tick, as Quadrants and Hemisphere do;
    // left set, every tick with a source high counts as a new trigger.
    HS::clock_m.auto_reset = false;

    for (int o = 0; o < kNumOutputs; ++o) {
      const Route &r = routes[o];
      const bool a = gate[r.a], b = gate[r.b];
      const bool rise_a = a && !prev_a[o], rise_b = b && !prev_b[o];
      prev_a[o] = a;
      prev_b[o] = b;
      // Function by CV: +5 V at 100% depth moves through the eight logic
      // functions; the M7 functions aren't modulated.
      int fn = r.fn;
      if (fn_cv[o].enabled() && fn < kNumLogicFns)
        fn = constrain(fn + fn_cv[o].In() * kNumLogicFns / (5 * ONE_OCTAVE), 0, kNumLogicFns - 1);
      live_fn[o] = fn;

      if (fn == FN_CNT || fn == FN_BIN) {
        int n = 0;
        for (int ch = 0; ch < kNumChannels; ++ch)
          n += (fn == FN_CNT) ? gate[ch] : (gate[ch] << ch);
        out_high[o] = n > 0;
        Out(o, n * kStepCV[out_step[o]]);
        continue;
      }
      if (fn == FN_ENV) {
        EnvTick(o, a, rise_a);
        out_high[o] = env_level[o] > 0;
        Out(o, env_level[o] >> 8);
        continue;
      }

      bool v;
      switch (fn) {
        case FN_TOGL: if (rise_a) toggled[o] = !toggled[o]; v = toggled[o]; break;
        case FN_LTCH: // set by a, reset by b; simultaneous edges are ignored
          if (rise_a != rise_b) toggled[o] = rise_a;
          v = toggled[o];
          break;
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
    ScopeSample();
#ifdef XANDNOR_AUDIO
    // Audio applets follow CV and triggers here too, except while a preset
    // load or save is changing the setup from the main loop.
    if (!audio_busy) audio_app.Controller();
#endif
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
  uint8_t prob[XandNorDefs::kNumChannels];      // 0-100%
  CVInputMap prob_cv[XandNorDefs::kNumChannels];
  uint8_t ratchets[XandNorDefs::kNumChannels];  // 1-8
  CVInputMap ratchet_cv[XandNorDefs::kNumChannels];
  int8_t div_steps[XandNorDefs::kNumChannels];  // the set divider; source's own is CV-adjusted
  CVInputMap div_cv[XandNorDefs::kNumChannels];
  uint8_t delay[XandNorDefs::kNumChannels];      // 0 = off, else a length-scale step
  CVInputMap delay_cv[XandNorDefs::kNumChannels];
  bool clock_length[XandNorDefs::kNumChannels];  // length is a % of the trigger period
  uint8_t clock_pct[XandNorDefs::kNumChannels];  // 1-100%
  bool level[XandNorDefs::kNumChannels];         // follow the source level, not edges
  uint8_t threshold_dv[XandNorDefs::kNumChannels]; // level threshold, 0.1 V units
  CVInputMap fn_cv[8];
  // Preset by CV. Global: stored with every patch, but loading a slot leaves it
  // alone, so a CV-driven load can't change what drives the next one.
  CVInputMap preset_cv;
  DigitalInputMap preset_trig;
  bool preset_audio = true; // preset loads include the audio setup (global)
  volatile bool audio_busy = false; // main loop is changing the audio setup
  volatile int8_t pending_slot = -1; // set by Controller(), loaded by Loop(); -2 = next
  XandNorDefs::Route routes[8];
  XandNorDefs::PackedPatch reserved_store; // keeps unknown reserved bytes on resave

  // Engine state, owned by Controller()
  struct ChannelState {
    uint32_t elapsed; // ticks since the gate started
    uint32_t total;   // gate length in ticks, sampled at the trigger (Sample mode)
    uint8_t drop;     // ticks left in a retrigger drop
    uint8_t ratchets; // pulses in this gate, sampled at the trigger (Sample mode)
    bool active;      // the gate is running (high, or dropping before going high again)
  } state[XandNorDefs::kNumChannels];
  // CV-modulated values, recomputed every tick (also shown on the channel page)
  uint32_t live_us[XandNorDefs::kNumChannels];
  uint8_t live_prob[XandNorDefs::kNumChannels];
  uint8_t live_ratchets[XandNorDefs::kNumChannels];
  uint32_t live_ticks[XandNorDefs::kNumChannels];  // gate length the engine uses
  // Tempo and delay state, owned by Controller()
  uint32_t last_clock[XandNorDefs::kNumChannels];
  uint32_t period[XandNorDefs::kNumChannels];      // ticks between triggers; 0 = unknown
  uint32_t delay_due[XandNorDefs::kNumChannels][XandNorDefs::kDelayQueue];
  uint8_t delay_count[XandNorDefs::kNumChannels];
  bool level_high[XandNorDefs::kNumChannels];
  uint8_t live_fn[8];
  // M7 per-output settings and state
  uint8_t out_step[8], env_mode[8], env_attack[8], env_decay[8];
  bool prev_a[8], prev_b[8], toggled[8];
  uint8_t env_phase[8]; // 0 idle, 1 attack, 2 decay/release, 3 sustain
  int32_t env_level[8]; // CV units << 8
  bool chan_high[XandNorDefs::kNumChannels];
  bool out_high[XandNorDefs::kNumOutputs];

  // UI
  int page = XandNorDefs::PAGE_OVERVIEW;
  int page_before_routing = XandNorDefs::PAGE_OVERVIEW; // for the X shortcut
  int cursor = 0;       // channel pages
  int scroll = 0;       // first visible channel-page row
  int route_cursor = 0; // routing page: output * ROUTE_FIELDS + field
  int mod_cursor = 0;   // CV Mod page
  int mod_scroll = 0;
  int out_cursor = 0;   // Outputs page: output * 3 + field
  int out_scroll = 0;

  // Presets
  XandNorDefs::PackedPatch slots[XandNorDefs::kNumSlots];
  uint8_t slot_used = 0;   // bit per slot
  uint8_t bank = 0;        // current bank; slots[] holds its contents
  bool bank_dirty = false; // slots need writing to the bank file (Loop)
  bool tidy_pending = true; // move internal-flash banks to the SD card (Loop)
  const char *bank_from = nullptr; // where the current bank loaded from
  int active_slot = -1;    // last slot saved or loaded; -1 = none
  bool preset_mode = false;
  int slot_cursor = 0;
  uint32_t message_until = 0; // millis() when the overlay message clears
  const char *message_badge = nullptr; // "[SD]"/"[Int]" after a bank message

  // Scope (not saved with patches)
  DigitalInputMap scope_trig;     // sweep trigger; none = free-running
  uint8_t scope_span = 0;         // index into kScopeSpanMs
  uint8_t scope_traces[XandNorDefs::kScopeTraces];
  int scope_cursor = 0;
  bool scope_hold = false;
  uint8_t scope_mode = XandNorDefs::SCOPE_SWEEP;
  uint8_t scope_head = 0;         // Roll: next column to write, i.e. the oldest
  uint8_t scope_zoom = 0;         // 8 >> zoom traces on screen: 8, 4, 2, 1
  uint8_t scope_win = 0;          // first trace on screen
  uint16_t scope_cap[XandNorDefs::kScopeCols];  // being captured (Controller)
  uint16_t scope_disp[XandNorDefs::kScopeCols]; // last complete sweep (display)
  int scope_col = -1;             // -1 = waiting for a trigger
  uint32_t scope_col_ticks = 0, scope_tick = 0;
  uint16_t scope_acc = 0;
  uint32_t scope_last_trig = 0, scope_period = 0;
  bool scope_peak = true;         // Peak or Sample capture
  // CV traces: each column's lowest and highest value (int8, kScopeCVScale)
  int8_t scope_cv_cap[2][XandNorDefs::kScopeCVInputs][XandNorDefs::kScopeCols];
  int8_t scope_cv_disp[2][XandNorDefs::kScopeCVInputs][XandNorDefs::kScopeCols];
  int8_t scope_cv_acc[2][XandNorDefs::kScopeCVInputs]; // [0] low, [1] high
  const char *message = "";

  // Advances channel ch by one tick and returns its gate.
  bool ProcessChannel(int ch) {
    using namespace XandNorDefs;
    ChannelState &s = state[ch];

    // Divider: the CV-adjusted value goes into the source before it's read.
    int div = div_steps[ch];
    if (div_cv[ch].enabled()) {
      const int k = DivStepsToIndex(div) + div_cv[ch].In() * kDivStepsPerVolt / ONE_OCTAVE;
      div = DivIndexToSteps(constrain(k, kDivIndexMin, kDivIndexMax));
    }
    source[ch].div_mult.steps = div;

    bool trig = source[ch].Clock(); // always called, to track edges
    const uint32_t now = OC::CORE::ticks;

    // Tempo: the time between triggers, measured before probability drops any.
    if (trig) {
      const uint32_t p = now - last_clock[ch];
      period[ch] = (last_clock[ch] && p <= kMaxPeriodTicks) ? p : 0;
      last_clock[ch] = now;
    }

    // A clock-relative length is always finite; Thru and Inf are time settings.
    const int len = clock_length[ch] ? 1 : length[ch];
    live_us[ch] = ModulatedLengthUs(ch);
    live_ticks[ch] = UsToTicks(live_us[ch]);
    if (clock_length[ch] && period[ch]) {
      const int pct = constrain(clock_pct[ch] + length_cv[ch].In() * 100 / (5 * ONE_OCTAVE), 1, 100);
      live_ticks[ch] = max(period[ch] * pct / 100, kMinGateTicks);
      live_us[ch] = live_ticks[ch] * OC_CORE_TIMER_RATE;
    }
    live_prob[ch] = constrain(prob[ch] + prob_cv[ch].In() * 100 / (5 * ONE_OCTAVE), 0, 100);
    live_ratchets[ch] = constrain(ratchets[ch] + ratchet_cv[ch].In() * (kMaxRatchets - 1) / (5 * ONE_OCTAVE),
                                  1, kMaxRatchets);

    // Probability: a dropped trigger starts nothing and leaves a running gate alone.
    if (trig && live_prob[ch] < 100 && random(100) >= live_prob[ch]) trig = false;

    if (len == kLengthThru) {
      s.active = false;
      s.drop = 0;
      delay_count[ch] = 0;
      if (level[ch]) {
        const int v = LevelIn(ch);
        const int thr = threshold_dv[ch] * ONE_OCTAVE / 10;
        if (v > thr + kHysteresis) level_high[ch] = true;
        else if (v < thr - kHysteresis) level_high[ch] = false;
        return chan_high[ch] = level_high[ch];
      }
      return chan_high[ch] = source[ch].Gate();
    }

    // Delay: queue the trigger; it fires when due. The delay is read at the
    // trigger. A full queue drops the newest.
    const uint32_t delay_ticks = DelayTicks(ch);
    if (trig && delay_ticks) {
      if (delay_count[ch] < kDelayQueue) delay_due[ch][delay_count[ch]++] = now + delay_ticks;
      trig = false;
    }
    if (delay_count[ch] && int32_t(now - delay_due[ch][0]) >= 0) {
      for (int i = 1; i < delay_count[ch]; ++i) delay_due[ch][i - 1] = delay_due[ch][i];
      --delay_count[ch];
      trig = true;
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

    if (!s.active) return chan_high[ch] = false;
    if (len == kLengthInfinite) return chan_high[ch] = true;

    const bool track = (cv_mode[ch] == CV_TRACK);
    const uint32_t total = track ? live_ticks[ch] : s.total;
    const uint32_t t = s.elapsed++;
    if (t >= total) {
      s.active = false;
      return chan_high[ch] = false;
    }
    // Ratchet: split the gate into n pulses at 50% duty, each at least 1 ms.
    uint32_t n = track ? live_ratchets[ch] : s.ratchets;
    n = constrain(n, 1u, total / kMinRatchetPeriod);
    if (n > 1) {
      const uint32_t period = total / n;
      // t past n whole periods is the division remainder: stay low, or it
      // would start a sliver of an extra pulse.
      return chan_high[ch] = t < period * n && (t % period) < period / 2;
    }
    return chan_high[ch] = true;
  }

  void StartGate(int ch, int len) {
    state[ch].active = true;
    state[ch].elapsed = 0;
    state[ch].total = live_ticks[ch];
    state[ch].ratchets = live_ratchets[ch];
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
    int len = length[ch];
    // Clock-relative with no tempo yet: fall back to the time setting, or the
    // default if that's Thru or Inf.
    if (clock_length[ch] && (len == kLengthThru || len == kLengthInfinite)) len = kDefaultLengths[ch];
    if (len == kLengthThru || len == kLengthInfinite) return 0;
    // position on the step scale in 1/256 steps
    const int32_t pos = len * 256 + length_cv[ch].In() * kStepsPerVolt * 256 / ONE_OCTAVE;
    return StepPosToUs(constrain(pos, 256, kNumLengths * 256));
  }

  // Delay in ticks with its CV applied; 0 = no delay. CV can open a delay
  // from Off, and pull one down to Off.
  uint32_t DelayTicks(int ch) const {
    using namespace XandNorDefs;
    if (!delay[ch] && !delay_cv[ch].enabled()) return 0;
    const int32_t pos = delay[ch] * 256 + delay_cv[ch].In() * kStepsPerVolt * 256 / ONE_OCTAVE;
    if (pos < 256) return 0;
    return UsToTicks(StepPosToUs(min(pos, int32_t(kNumLengths * 256))));
  }

  // One envelope tick. AD: a rising edge runs attack then decay. ASR: attack
  // on the rise, sustain while the gate is high, release (the decay time) when
  // it falls. Segments are linear in voltage.
  void EnvTick(int o, bool gate_high, bool rise) {
    using namespace XandNorDefs;
    const int32_t up = max(kEnvMax / int32_t(UsToTicks(kLengthUs[env_attack[o] - 1])), 1);
    const int32_t down = max(kEnvMax / int32_t(UsToTicks(kLengthUs[env_decay[o] - 1])), 1);
    if (rise) env_phase[o] = 1;
    switch (env_phase[o]) {
      case 1: // attack
        env_level[o] += up;
        if (env_level[o] >= kEnvMax) {
          env_level[o] = kEnvMax;
          env_phase[o] = (env_mode[o] == EMODE_ASR) ? 3 : 2;
        }
        if (env_mode[o] == EMODE_ASR && !gate_high) env_phase[o] = 2;
        break;
      case 3: // sustain (ASR)
        if (!gate_high) env_phase[o] = 2;
        break;
      case 2: // decay / release
        env_level[o] -= down;
        if (env_level[o] <= 0) {
          env_level[o] = 0;
          env_phase[o] = 0;
        }
        break;
      default:
        break;
    }
  }

  // One tick of scope capture: OR this tick's states into the current column;
  // start a sweep on the trigger (or at once when free-running).
  void ScopeSample() {
    using namespace XandNorDefs;
    uint16_t mask = 0;
    for (int ch = 0; ch < kNumChannels; ++ch) mask |= chan_high[ch] << ch;
    for (int o = 0; o < kNumOutputs; ++o) mask |= out_high[o] << (4 + o);

    const bool free_run = !(scope_trig.Pack() & 0xff);
    const bool trig = scope_trig.Clock();
    const uint32_t now = OC::CORE::ticks;
    if (trig) {
      const uint32_t p = now - scope_last_trig;
      scope_period = (scope_last_trig && p <= kMaxPeriodTicks) ? p : 0;
      scope_last_trig = now;
    }

    // Triggered: wait for the trigger (or start at once when free-running).
    // Roll: never wait; columns run continuously into a ring.
    if (scope_col < 0) {
      if (scope_mode == SCOPE_TRIGGERED && !trig && !free_run) return;
      // Ticks per column: the set span, or Auto = two trigger periods.
      uint32_t span = kScopeSpanMs[scope_span] * 16666u / 1000; // ms -> ticks
      if (!span) {
        if (scope_mode == SCOPE_ROLL) span = kRollAutoSpanMs * 16666u / 1000;
        else span = (scope_period && !free_run) ? 2 * scope_period : kScopeFreeSpanMs * 16666u / 1000;
      }
      scope_col_ticks = max(span / kScopeCols, 1u);
      scope_col = 0;
      scope_tick = 0;
      scope_acc = 0;
      for (int i = 0; i < kScopeCVInputs; ++i) {
        scope_cv_acc[0][i] = 127;
        scope_cv_acc[1][i] = -128;
      }
    }
    // Peak keeps every pulse and each CV's range; Sample keeps the last tick.
    const bool sample = !scope_peak;
    scope_acc = sample ? mask : (scope_acc | mask);
    for (int i = 0; i < kScopeCVInputs; ++i) {
      const int8_t v = constrain(HS::frame.In(i) / kScopeCVScale, -128, 127);
      if (sample || v < scope_cv_acc[0][i]) scope_cv_acc[0][i] = v;
      if (sample || v > scope_cv_acc[1][i]) scope_cv_acc[1][i] = v;
    }
    if (++scope_tick < scope_col_ticks) return;

    if (scope_mode != SCOPE_TRIGGERED) { // Roll and Sweep write a ring
      if (!scope_hold) {
        scope_disp[scope_head] = scope_acc;
        for (int i = 0; i < kScopeCVInputs; ++i) {
          scope_cv_disp[0][i][scope_head] = scope_cv_acc[0][i];
          scope_cv_disp[1][i][scope_head] = scope_cv_acc[1][i];
        }
        scope_head = (scope_head + 1) % kScopeCols;
      }
      scope_col = -1; // recompute the column time each column
      return;
    }
    for (int i = 0; i < kScopeCVInputs; ++i) {
      scope_cv_cap[0][i][scope_col] = scope_cv_acc[0][i];
      scope_cv_cap[1][i][scope_col] = scope_cv_acc[1][i];
      scope_cv_acc[0][i] = 127;
      scope_cv_acc[1][i] = -128;
    }
    scope_cap[scope_col++] = scope_acc;
    scope_acc = 0;
    scope_tick = 0;
    if (scope_col >= kScopeCols) {
      if (!scope_hold) {
        memcpy(scope_disp, scope_cap, sizeof(scope_disp));
        memcpy(scope_cv_disp, scope_cv_cap, sizeof(scope_cv_disp));
      }
      scope_col = -1;
    }
  }

  // The level of a channel's source, for level mode: the CV of an input or
  // output, or 5 V / 0 V for a trigger or MIDI source.
  int LevelIn(int ch) const {
    const int idx = source[ch].index();
    switch (source[ch].source_type()) {
      case DigitalInputMap::TYPE_ADC: return HS::frame.In(idx);
      case DigitalInputMap::TYPE_DAC: return HS::frame.ViewOut(idx);
      default: return source[ch].Gate() ? 5 * ONE_OCTAVE : 0;
    }
  }

  // A position on the length scale in 1/256 steps (256 = 1 ms) to microseconds,
  // interpolating linearly between table steps.
  static uint32_t StepPosToUs(int32_t pos) {
    using namespace XandNorDefs;
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
  void PrintDepth(int x, int y, const CVInputMap &m) const;
  void PrintDiv(int x, int y, int steps) const;
  void DrawArrow(int x, int y) const;
  void PrintRoute(int x, int y, const XandNorDefs::Route &r) const;
  void DrawPresets() const;

  void LoadDefaults();
  void PackPatch(XandNorDefs::PackedPatch &p) const;
  void UnpackPatch(const XandNorDefs::PackedPatch &p, bool with_global = false);
  void DrawMod() const;
  void DrawOutputs() const;
  void DrawScope(bool interactive = true) const;
  void ScopeFollowCursor();
  void DrawScrollBar(int scroll_pos, int rows) const;
  void SaveSlot(int slot);
  void LoadSlot(int slot);
  bool WriteBank();
  bool WriteBankTo(FS &fs);
  int ReadBank(int b);
  int ReadBankFrom(FS &fs, int b);
  void TidyInternalBanks();
#ifdef XANDNOR_AUDIO
  bool SaveSlotAudio(int slot);
  void LoadSlotAudio(int slot);
#endif
  void SelectBank(int b);
};

// -- Setup -------------------------------------------------------------------

FLASHMEM void XandNor::LoadDefaults() {
  using namespace XandNorDefs;
  // Unpack rather than Clear(): Clear() resets edge detection, so a high input
  // would fire every channel once. t1 must be value-initialised: its Euclidean
  // fields (e_length etc.) have no initialisers, and stack garbage there makes
  // Clock()'s Euclidean filter swallow every trigger.
  DigitalInputMap t1{};
  t1.SetGateInput(0);
  for (int ch = 0; ch < kNumChannels; ++ch) {
    source[ch].Unpack(t1.Pack());
    length[ch] = kDefaultLengths[ch];
    length_cv[ch] = CVInputMap();
    cv_mode[ch] = CV_SAMPLE;
    prob[ch] = 100;
    prob_cv[ch] = CVInputMap();
    ratchets[ch] = 1;
    ratchet_cv[ch] = CVInputMap();
    div_steps[ch] = 1;
    div_cv[ch] = CVInputMap();
    delay[ch] = 0;
    delay_cv[ch] = CVInputMap();
    clock_length[ch] = false;
    clock_pct[ch] = kDefaultClockPct;
    level[ch] = false;
    threshold_dv[ch] = kDefaultThresholdDv;
  }
  for (int o = 0; o < 8; ++o) {
    fn_cv[o] = CVInputMap();
    out_step[o] = CVSTEP_SEMI;
    env_mode[o] = EMODE_AD;
    env_attack[o] = kDefaultAttack;
    env_decay[o] = kDefaultDecay;
  }
  memset(&reserved_store, 0, sizeof(reserved_store));
  for (int o = 0; o < 8; ++o) routes[o] = kDefaultRoutes[o];
}

FLASHMEM void XandNor::Start() {
  using namespace XandNorDefs;
  LoadDefaults();
  for (int ch = 0; ch < kNumChannels; ++ch) {
    state[ch] = {0, 0, 0, 1, false};
    live_us[ch] = 0;
    live_prob[ch] = 100;
    live_ratchets[ch] = 1;
    live_ticks[ch] = 0;
    last_clock[ch] = 0;
    period[ch] = 0;
    delay_count[ch] = 0;
    level_high[ch] = false;
    chan_high[ch] = false;
  }
  for (int o = 0; o < kNumOutputs; ++o) out_high[o] = false;
  bank = 0;
  bank_dirty = false;
  ReadBank(0); // RestoreAppData() switches to the saved bank
  active_slot = -1;
  page = PAGE_OVERVIEW;
  cursor = 0;
  scroll = 0;
  route_cursor = 0;
  mod_cursor = 0;
  mod_scroll = 0;
  out_cursor = 0;
  out_scroll = 0;
  for (int o = 0; o < 8; ++o) {
    prev_a[o] = prev_b[o] = toggled[o] = false;
    env_phase[o] = 0;
    env_level[o] = 0;
  }
  for (int o = 0; o < 8; ++o) live_fn[o] = routes[o].fn;
  preset_cv = CVInputMap();
  preset_trig = DigitalInputMap{};
  scope_trig = DigitalInputMap{};
  scope_trig.SetGateInput(0); // T1
  scope_span = 0;
  for (int i = 0; i < kScopeTraces; ++i) scope_traces[i] = kDefaultTraces[i];
  scope_cursor = 0;
  scope_hold = false;
  scope_mode = SCOPE_SWEEP;
  scope_head = 0;
  scope_zoom = 0;
  scope_win = 0;
  scope_col = -1;
  scope_peak = true;
  memset(scope_disp, 0, sizeof(scope_disp));
  memset(scope_cv_disp[0], 127, sizeof(scope_cv_disp[0])); // low > high = empty
  memset(scope_cv_disp[1], -128, sizeof(scope_cv_disp[1]));
  pending_slot = -1;
}

// -- Presets -----------------------------------------------------------------

FLASHMEM void XandNor::PackPatch(XandNorDefs::PackedPatch &p) const {
  using namespace XandNorDefs;
  p = reserved_store; // carry reserved bytes through unchanged
  for (int ch = 0; ch < kNumChannels; ++ch) {
    DigitalInputMap m = source[ch];
    m.div_mult.steps = div_steps[ch]; // the set divider, not the CV-adjusted one
    p.source[ch] = m.Pack();
    p.length[ch] = length[ch];
    ChannelExtras &e = p.extras[ch];
    e.length_cv = length_cv[ch].Pack();
    e.cv_mode = cv_mode[ch];
    e.prob_skip = 100 - prob[ch];
    e.prob_cv = prob_cv[ch].Pack();
    e.ratchet_extra = ratchets[ch] - 1;
    e.ratchet_cv = ratchet_cv[ch].Pack();
    e.div_cv = div_cv[ch].Pack();
    e.delay = delay[ch];
    e.delay_cv = delay_cv[ch].Pack();
    e.clock_pct = clock_pct[ch];
    e.flags = (clock_length[ch] ? kFlagClockLength : 0) | (level[ch] ? kFlagLevel : 0)
            | uint8_t(threshold_dv[ch] << 2);
  }
  for (int o = 0; o < 8; ++o) {
    p.fn_cv[o] = fn_cv[o].Pack();
    p.out[o].step = out_step[o];
    p.out[o].env_mode = env_mode[o];
    p.out[o].attack = env_attack[o];
    p.out[o].decay = env_decay[o];
  }
  p.preset_cv = preset_cv.Pack();
  p.preset_trig = preset_trig.Pack() & 0xff; // source only
  p.ext_reserved[0] = preset_audio ? 0 : 1;  // stored inverted: zero = default On
  for (int o = 0; o < 8; ++o) p.routes[o] = routes[o];
}

// A zero stored CV map means "never set" (bytes reserved before the field
// existed): use the default, whose depth is 100% rather than Unpack(0)'s 0%.
FLASHMEM static void UnpackCV(CVInputMap &m, uint16_t packed) {
  if (packed) m.Unpack(packed);
  else m = CVInputMap();
}

// Validates as it goes, so a corrupt or foreign slot can't index out of range.
FLASHMEM void XandNor::UnpackPatch(const XandNorDefs::PackedPatch &p, bool with_global) {
  using namespace XandNorDefs;
  reserved_store = p;
  for (int ch = 0; ch < kNumChannels; ++ch) {
    source[ch].Unpack(p.source[ch]);
    length[ch] = constrain(p.length[ch], kLengthThru, kLengthInfinite);
    div_steps[ch] = source[ch].div_mult.steps; // Unpack never leaves 0
    const ChannelExtras &e = p.extras[ch];
    UnpackCV(length_cv[ch], e.length_cv);
    cv_mode[ch] = constrain(e.cv_mode, 0, CV_MODE_COUNT - 1);
    prob[ch] = 100 - constrain(e.prob_skip, 0, 100);
    UnpackCV(prob_cv[ch], e.prob_cv);
    ratchets[ch] = 1 + constrain(e.ratchet_extra, 0, kMaxRatchets - 1);
    UnpackCV(ratchet_cv[ch], e.ratchet_cv);
    UnpackCV(div_cv[ch], e.div_cv);
    delay[ch] = constrain(e.delay, 0, kNumLengths);
    UnpackCV(delay_cv[ch], e.delay_cv);
    clock_pct[ch] = e.clock_pct ? constrain(e.clock_pct, 1, 100) : kDefaultClockPct;
    clock_length[ch] = e.flags & kFlagClockLength;
    level[ch] = e.flags & kFlagLevel;
    threshold_dv[ch] = (e.flags >> 2) ? (e.flags >> 2) : kDefaultThresholdDv;
    if (level[ch]) length[ch] = kLengthThru; // level mode lives below Thru
  }
  for (int o = 0; o < 8; ++o) {
    UnpackCV(fn_cv[o], p.fn_cv[o]);
    const PackedPatch::OutputExtras &x = p.out[o];
    out_step[o] = constrain(x.step, 0, CVSTEP_COUNT - 1);
    env_mode[o] = constrain(x.env_mode, 0, EMODE_COUNT - 1);
    env_attack[o] = x.attack ? constrain(x.attack, 1, kNumLengths) : kDefaultAttack;
    env_decay[o] = x.decay ? constrain(x.decay, 1, kNumLengths) : kDefaultDecay;
  }
  if (with_global) {
    UnpackCV(preset_cv, p.preset_cv);
    DigitalInputMap t{}; // value-initialised: see LoadDefaults()
    t.Unpack(p.preset_trig);
    preset_trig = t;
    preset_audio = !(p.ext_reserved[0] & 1);
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
  WriteBank(); // sets the message and its [SD]/[Int] badge
#ifdef XANDNOR_AUDIO
  if (!SaveSlotAudio(slot)) { message = "Audio not saved"; message_badge = nullptr; }
#endif
  message_until = millis() + XandNorDefs::kMessageMs;
}

// An empty slot loads the default patch.
FLASHMEM void XandNor::LoadSlot(int slot) {
  if (slot_used & (1 << slot)) {
    UnpackPatch(slots[slot]);
#ifdef XANDNOR_AUDIO
    if (preset_audio) LoadSlotAudio(slot);
#endif
  } else {
    LoadDefaults();
  }
  active_slot = slot;
  message = (slot_used & (1 << slot)) ? "Loaded" : "Defaults";
  message_badge = nullptr;
  message_until = millis() + XandNorDefs::kMessageMs;
}

// -- Banks -------------------------------------------------------------------
// Banks are files of eight slots, on the SD card when one is ready, else the
// platform's internal LittleFS. Saving a slot rewrites its bank's file.

FLASHMEM static void BankFileName(char *name, int b) {
  strcpy(name, "XNBANK00.DAT");
  name[6] = '0' + b / 10;
  name[7] = '0' + b % 10;
}

// Writes the current bank to one file system, then reads it back and compares,
// so a card that accepts writes but loses them on close counts as a failure.
FLASHMEM bool XandNor::WriteBankTo(FS &fs) {
  using namespace XandNorDefs;
  char name[13];
  BankFileName(name, bank);
  fs.remove(name);
  File f = fs.open(name, FILE_WRITE_BEGIN);
  if (!f) return false;
  const BankHeader h = {{'X', 'N', 'B', 'K'}, kBankVersion, slot_used, {0, 0}};
  bool ok = f.write(&h, sizeof(h)) == sizeof(h);
  ok = ok && f.write(slots, sizeof(slots)) == sizeof(slots);
  f.close();
  if (!ok) return false;

  f = fs.open(name, FILE_READ);
  if (!f) return false;
  BankHeader rh;
  ok = f.read(&rh, sizeof(rh)) == sizeof(rh) && !memcmp(&rh, &h, sizeof(h));
  uint8_t buf[sizeof(PackedPatch)];
  for (int i = 0; ok && i < kNumSlots; ++i)
    ok = f.read(buf, sizeof(buf)) == sizeof(buf) && !memcmp(buf, &slots[i], sizeof(buf));
  f.close();
  return ok;
}

// SD card first; if that fails (or there's no card), the internal flash.
FLASHMEM bool XandNor::WriteBank() {
  if (SDcard_Ready && WriteBankTo(SD)) {
    // The SD copy now wins on load, so any internal copy is stale.
    char name[13];
    BankFileName(name, bank);
    PhzConfig::myfs.remove(name);
    message = "Saved";
    message_badge = "[SD]";
    return true;
  }
  if (WriteBankTo(PhzConfig::myfs)) {
    message = "Saved";
    message_badge = "[Int]";
    return true;
  }
  message = "Save failed";
  message_badge = nullptr;
  return false;
}

// With an SD card present, moves bank files out of the internal flash: copies
// any the card lacks (verifying the copy) and deletes the internal file. If the
// card already has the bank, the internal copy is stale and just goes. A copy
// that fails verification leaves the internal file in place.
// Copies one file from the internal flash to the SD card in chunks, reading
// the copy back to compare, then deletes the internal file. A file the card
// already has is stale internally and just goes. A failed copy keeps the
// internal file and removes the partial one from the card.
FLASHMEM static void MoveFileToSD(const char *name) {
  if (!PhzConfig::myfs.exists(name)) return;
  if (!SD.exists(name)) {
    uint8_t buf[256], check[256];
    File in = PhzConfig::myfs.open(name, FILE_READ);
    File out = SD.open(name, FILE_WRITE_BEGIN);
    bool ok = in && out;
    int n;
    while (ok && (n = in.read(buf, sizeof(buf))) > 0) ok = out.write(buf, n) == size_t(n);
    if (out) out.close();
    if (ok) { // compare the copy with the original
      in.seek(0);
      File back = SD.open(name, FILE_READ);
      ok = bool(back);
      while (ok && (n = in.read(buf, sizeof(buf))) > 0)
        ok = int(back.read(check, n)) == n && !memcmp(check, buf, n);
      if (back) back.close();
    }
    if (in) in.close();
    if (!ok) {
      SD.remove(name);
      return;
    }
  }
  PhzConfig::myfs.remove(name);
}

FLASHMEM static void AudioFileName(char *name, int b) {
  strcpy(name, "XNAUD00.DAT");
  name[5] = '0' + b / 10;
  name[6] = '0' + b % 10;
}

FLASHMEM void XandNor::TidyInternalBanks() {
  using namespace XandNorDefs;
  if (!SDcard_Ready) return;
  char name[13];
  for (int b = 0; b < kNumBanks; ++b) {
    BankFileName(name, b);
    MoveFileToSD(name);
    AudioFileName(name, b);
    MoveFileToSD(name);
  }
}

#ifdef XANDNOR_AUDIO
// -- Audio in presets (M8b) ---------------------------------------------------
// The audio engine keeps its setup private, but saves and loads presets through
// the platform config store (PhzConfig), keyed by preset number. Quadrants uses
// all 32 numbers, so the audio keys of one (kScratchPreset) are backed up, used
// to capture or apply a setup, and restored, leaving Quadrants' data unchanged.
// A setup is the 76 audio keys: the stereo flags, an applet ID per slot and
// side, and each applet's settings. Each bank has a companion file of eight.
namespace XandNorAudio {
static constexpr int kScratchPreset = 31;
static constexpr int kCfg = HemisphereAudioApplet::CONFIG_SIZE;
static constexpr int kSlots = NUM_SLOTS;
static constexpr int kKeys = 1 + 2 * kSlots + kSlots + 2 * kSlots * kCfg + kSlots * kCfg;
// The i-th audio key below the preset number: (section << 8) | index, sections
// as in AudioAppletSubapp: 1 main, 2 mono IDs, 3 stereo IDs, 4 mono and 5
// stereo applet settings.
static constexpr uint16_t Key(int i) {
  return i < 1 ? (1 << 8)
       : i < 1 + 2 * kSlots ? ((2 << 8) | (i - 1))
       : i < 1 + 3 * kSlots ? ((3 << 8) | (i - 1 - 2 * kSlots))
       : i < 1 + 3 * kSlots + 2 * kSlots * kCfg ? ((4 << 8) | (i - 1 - 3 * kSlots))
       : ((5 << 8) | (i - 1 - 3 * kSlots - 2 * kSlots * kCfg));
}
static_assert(kKeys == 76, "audio key count changed: check AudioAppletSubapp's layout");
struct Snapshot { uint64_t v[kKeys]; }; // 0 = key absent
struct File_ {
  char magic[4];   // "XNAU"
  uint8_t version; // 1
  uint8_t valid;   // bit per slot holding a setup
  uint8_t pad[2];
  Snapshot slot[XandNorDefs::kNumSlots];
};
DMAMEM static File_ file; // one bank's audio, read and written whole
static constexpr uint16_t Base() { return uint16_t(kScratchPreset) << 11; }

FLASHMEM static void Backup(uint64_t *saved, bool *had) {
  for (int i = 0; i < kKeys; ++i) had[i] = PhzConfig::getValue(Base() | Key(i), saved[i]);
}
FLASHMEM static void Restore(const uint64_t *saved, const bool *had) {
  for (int i = 0; i < kKeys; ++i) {
    if (had[i]) PhzConfig::setValue(Base() | Key(i), saved[i]);
    else PhzConfig::deleteKey(Base() | Key(i));
  }
}
FLASHMEM static void Capture(Snapshot &s) {
  uint64_t saved[kKeys]; bool had[kKeys];
  Backup(saved, had);
  audio_app.SavePreset(kScratchPreset);
  for (int i = 0; i < kKeys; ++i)
    if (!PhzConfig::getValue(Base() | Key(i), s.v[i])) s.v[i] = 0;
  Restore(saved, had);
}
FLASHMEM static void Apply(const Snapshot &s) {
  uint64_t saved[kKeys]; bool had[kKeys];
  Backup(saved, had);
  for (int i = 0; i < kKeys; ++i) {
    if (s.v[i]) PhzConfig::setValue(Base() | Key(i), s.v[i]);
    else PhzConfig::deleteKey(Base() | Key(i));
  }
  audio_app.LoadPreset(kScratchPreset);
  Restore(saved, had);
}

FLASHMEM static bool ReadFrom(FS &fs, const char *name) {
  File f = fs.open(name, FILE_READ);
  if (!f) return false;
  const bool ok = f.read(&file, sizeof(file)) == sizeof(file) &&
                  !memcmp(file.magic, "XNAU", 4) && file.version == 1;
  f.close();
  return ok;
}
// Loads bank b's audio file (SD first); a missing or bad file is all empty.
FLASHMEM static void Read(int b) {
  char name[13];
  AudioFileName(name, b);
  if (!(SDcard_Ready && ReadFrom(SD, name)) && !ReadFrom(PhzConfig::myfs, name)) {
    memset(&file, 0, sizeof(file));
    memcpy(file.magic, "XNAU", 4);
    file.version = 1;
  }
}
FLASHMEM static bool WriteTo(FS &fs, const char *name) {
  fs.remove(name);
  File f = fs.open(name, FILE_WRITE_BEGIN);
  if (!f) return false;
  bool ok = f.write(&file, sizeof(file)) == sizeof(file);
  f.close();
  if (!ok) return false;
  // read back and compare in chunks
  f = fs.open(name, FILE_READ);
  if (!f) return false;
  uint8_t check[256];
  const uint8_t *src = reinterpret_cast<const uint8_t *>(&file);
  for (size_t done = 0; ok && done < sizeof(file); done += sizeof(check)) {
    const size_t n = min(sizeof(check), sizeof(file) - done);
    ok = size_t(f.read(check, n)) == n && !memcmp(check, src + done, n);
  }
  f.close();
  return ok;
}
// Writes bank b's audio file: SD first (then any internal copy is stale), else
// the internal flash.
FLASHMEM static bool Write(int b) {
  char name[13];
  AudioFileName(name, b);
  if (SDcard_Ready && WriteTo(SD, name)) {
    PhzConfig::myfs.remove(name);
    return true;
  }
  return WriteTo(PhzConfig::myfs, name);
}
} // namespace XandNorAudio

FLASHMEM bool XandNor::SaveSlotAudio(int slot) {
  XandNorAudio::Read(bank);
  audio_busy = true;
  XandNorAudio::Capture(XandNorAudio::file.slot[slot]);
  audio_busy = false;
  XandNorAudio::file.valid |= 1 << slot;
  return XandNorAudio::Write(bank);
}

FLASHMEM void XandNor::LoadSlotAudio(int slot) {
  XandNorAudio::Read(bank);
  if (XandNorAudio::file.valid & (1 << slot)) {
    audio_busy = true;
    XandNorAudio::Apply(XandNorAudio::file.slot[slot]);
    audio_busy = false;
  }
}
#endif

// Reads bank b from one file system: 1 = loaded, 0 = no file, -1 = bad file.
FLASHMEM int XandNor::ReadBankFrom(FS &fs, int b) {
  using namespace XandNorDefs;
  char name[13];
  BankFileName(name, b);
  File f = fs.open(name, FILE_READ);
  if (!f) return 0;
  BankHeader h;
  const bool ok = f.read(&h, sizeof(h)) == sizeof(h) && !memcmp(h.magic, "XNBK", 4) &&
                  h.version == kBankVersion && f.read(slots, sizeof(slots)) == sizeof(slots);
  f.close();
  if (!ok) return -1;
  slot_used = h.slot_used;
  return 1;
}

// Loads bank b into the slots, from the SD card first, then internal flash.
// No file means an empty bank. Returns ReadBankFrom()'s result.
FLASHMEM int XandNor::ReadBank(int b) {
  using namespace XandNorDefs;
  memset(slots, 0, sizeof(slots));
  slot_used = 0;
  int r = SDcard_Ready ? ReadBankFrom(SD, b) : 0;
  bank_from = (r == 1) ? "[SD]" : nullptr;
  if (r != 1) {
    const int r2 = ReadBankFrom(PhzConfig::myfs, b);
    if (r2 != 0) r = r2;
    if (r2 == 1) bank_from = "[Int]";
  }
  if (r != 1) { // nothing usable: leave the bank empty
    memset(slots, 0, sizeof(slots));
    slot_used = 0;
  }
  return r;
}

FLASHMEM void XandNor::SelectBank(int b) {
  bank = b;
  const int r = ReadBank(b);
  active_slot = -1;
  if (r < 0) {
    message = "Read error";
  } else if (!slot_used) {
    message = "Empty bank";
  } else {
    // "N saved" for the slots this bank holds
    static char text[12];
    int n = 0;
    for (int i = 0; i < XandNorDefs::kNumSlots; ++i) n += (slot_used >> i) & 1;
    text[0] = '0' + n;
    strcpy(text + 1, " saved");
    message = text;
  }
  message_badge = slot_used ? bank_from : nullptr;
  message_until = millis() + XandNorDefs::kMessageMs;
}

// -- UI ----------------------------------------------------------------------

FLASHMEM void XandNor::View() const {
  using namespace XandNorDefs;
  if (preset_mode) {
    DrawPresets();
    return;
  }
#ifdef XANDNOR_AUDIO
  if (page == PAGE_AUDIO) { // the audio engine draws its own full screen
    audio_app.View();
    return;
  }
#endif
  if (page == PAGE_SCOPE) { // full screen: no header or page dots
    DrawScope();
    return;
  }
  gfxHeader("(X)AND(N)OR");
  gfxPrint(128 - 6 * (int)strlen(kPageNames[page]) - 1, 1, kPageNames[page]);

  switch (page) {
    case PAGE_OVERVIEW: DrawOverview(); break;
    case PAGE_ROUTING:  DrawRouting(); break;
    case PAGE_CVMOD:      DrawMod(); break;
    case PAGE_OUTPUTS:    DrawOutputs(); break;
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

FLASHMEM void XandNor::PrintDepth(int x, int y, const CVInputMap &m) const {
  const int tenths = Atten(m.attenuversion);
  gfxPos(x, y);
  if (tenths < 0) gfxPrint("-");
  gfxPrint(abs(tenths) / 10);
  gfxPrint("%");
}

// "/2" divides, "x2" multiplies. Every trigger passing shows as "x1", which
// reads as a rate setting better than "/1".
FLASHMEM void XandNor::PrintDiv(int x, int y, int steps) const {
  gfxPrint(x, y, steps > 1 ? "/" : "x");
  gfxPrint(abs(steps));
}

// Small right arrow before a CV-adjusted live value.
FLASHMEM void XandNor::DrawArrow(int x, int y) const {
  gfxLine(x, y + 3, x + 4, y + 3);
  gfxLine(x + 3, y + 2, x + 3, y + 4);
}

FLASHMEM void XandNor::DrawChannelPage(int ch) const {
  using namespace XandNorDefs;
  // Live gate: a small box in the header, left of the page name.
  gfxFrame(93, 1, 8, 8);
  if (chan_high[ch]) gfxRect(94, 2, 6, 6);

  // A scrolling list: rows scroll so the cursor's row stays in view.
  for (int r = scroll; r < scroll + kVisibleRows && r < kChannelRows; ++r) {
    const int y = 13 + (r - scroll) * 9;
    // In level mode the "Len by" row sets the threshold instead.
    gfxPrint(1, y, (r == 3 && level[ch]) ? "Thresh:" : kRowLabels[r]);
    switch (r) {
      case 0:
        gfxPrint(52, y, source[ch].InputName());
        PrintDiv(76, y, div_steps[ch]);
        if (div_cv[ch].enabled() && source[ch].div_mult.steps != div_steps[ch]) {
          DrawArrow(96, y);
          PrintDiv(102, y, source[ch].div_mult.steps);
        }
        break;
      case 1:
        gfxPos(52, y);
        if (clock_length[ch]) {
          gfxPrint(clock_pct[ch]);
          gfxPrint("%");
        } else if (level[ch]) {
          gfxPrint("Level");
        } else {
          PrintLength(length[ch]);
        }
        // The length in use: CV-adjusted, or the % of the measured period.
        if ((length_cv[ch].enabled() || clock_length[ch]) && live_us[ch]) {
          DrawArrow(86, y);
          gfxPos(94, y);
          PrintMicros(live_us[ch]);
        }
        break;
      case 2:
        gfxPrint(52, y, length_cv[ch].InputName());
        PrintDepth(88, y, length_cv[ch]);
        break;
      case 3:
        if (level[ch]) {
          gfxPrint(52, y, threshold_dv[ch] / 10);
          gfxPrint(".");
          gfxPrint(threshold_dv[ch] % 10);
          gfxPrint("V");
        } else {
          gfxPrint(52, y, clock_length[ch] ? "Clock" : "Time");
        }
        break;
      case 4: {
        gfxPos(52, y);
        if (delay[ch]) PrintMicros(kLengthUs[delay[ch] - 1]);
        else gfxPrint("Off");
        if (delay_cv[ch].enabled()) {
          DrawArrow(86, y);
          const uint32_t t = DelayTicks(ch);
          gfxPos(94, y);
          if (t) PrintMicros(t * OC_CORE_TIMER_RATE);
          else gfxPrint("Off");
        }
        break;
      }
      case 5:
        gfxPrint(52, y, delay_cv[ch].InputName());
        PrintDepth(88, y, delay_cv[ch]);
        break;
      case 6:
        gfxPrint(52, y, prob[ch]);
        gfxPrint("%");
        if (prob_cv[ch].enabled()) {
          DrawArrow(86, y);
          gfxPrint(94, y, live_prob[ch]);
          gfxPrint("%");
        }
        break;
      case 7:
        gfxPrint(52, y, prob_cv[ch].InputName());
        PrintDepth(88, y, prob_cv[ch]);
        break;
      case 8:
        gfxPrint(52, y, ratchets[ch]);
        if (ratchet_cv[ch].enabled()) {
          DrawArrow(86, y);
          gfxPrint(94, y, live_ratchets[ch]);
        }
        break;
      case 9:
        gfxPrint(52, y, ratchet_cv[ch].InputName());
        PrintDepth(88, y, ratchet_cv[ch]);
        break;
      case 10:
        gfxPrint(52, y, div_cv[ch].InputName());
        PrintDepth(88, y, div_cv[ch]);
        break;
      default:
        gfxPrint(52, y, kCVModeNames[cv_mode[ch]]);
        break;
    }
  }

  DrawScrollBar(scroll, kChannelRows);

  const FieldPos &f = kFieldPos[cursor];
  DrawFieldCursor(f.x - 1, 13 + (f.row - scroll) * 9 + 8, f.w);
}

// Scope: a status line, then eight 7-pixel traces with labels. The selected
// trace has a bar beside its label; the selected status field is underlined.
// The interactive page has a status line and marks the selected trace by
// inverting its label; the screensaver gives the traces the whole screen.
FLASHMEM void XandNor::DrawScope(bool interactive) const {
  using namespace XandNorDefs;
  int top = 0;
  if (interactive) {
    top = 8;
    gfxPrint(1, 0, kScopeModeNames[scope_mode]);
    gfxPrint(30, 0, scope_trig.InputName());
    gfxPos(54, 0);
    const uint16_t ms = kScopeSpanMs[scope_span];
    if (!ms) {
      gfxPrint("Auto");
    } else if (ms < 1000) {
      gfxPrint(ms); gfxPrint("ms");
    } else {
      gfxPrint(ms / 1000);
      if (ms % 1000) { gfxPrint("."); gfxPrint(ms % 1000 / 100); }
      gfxPrint("s");
    }
    gfxPrint(88, 0, scope_peak ? "Pk" : "Sm");
    if (scope_hold) gfxPrint(104, 0, "HOLD");
  }

  // Zoom: 8, 4, 2 or 1 traces share the space below the status line.
  const int visible = kScopeTraces >> scope_zoom;
  const int h = (64 - top) / visible;
  for (int i = scope_win; i < scope_win + visible; ++i) {
    const int y = top + (i - scope_win) * h;
    const int sig = scope_traces[i];
    if (interactive) { // the screensaver drops the labels
      if (!sig) gfxPrint(1, y, "--");
      else if (sig >= kScopeFirstCV) { gfxPrint(1, y, "V"); gfxPrint(sig - kScopeFirstCV + 1); }
      else if (sig <= kNumChannels) { gfxPrint(1, y, "C"); gfxPrint(sig); }
      else { const char n[2] = { char('A' + sig - 1 - kNumChannels), '\0' }; gfxPrint(4, y, n); }
      if (scope_cursor == SCOPE_TRACE0 + i) gfxInvert(0, y, 13, 7);
    }
    if (!sig) continue;

    if (sig >= kScopeFirstCV) { // a CV input: its range in each column
      const int in = sig - kScopeFirstCV;
      const int first = scope_mode == SCOPE_ROLL ? scope_head : 0;
      const int x0 = interactive ? 14 : 0;
      const int width = interactive ? kScopeCols : 128;
      const int mid = y + h / 2, half = max(h / 2 - 1, 1);
      auto Y = [&](int v) { return constrain(mid - v * half / kScopeCVRange, y + 1, y + h - 2); };
      int prev_y = -1;
      for (int px = 0; px < width; ++px) {
        const int c = (first + (interactive ? px : px * kScopeCols / 128)) % kScopeCols;
        const int lo = scope_cv_disp[0][in][c], hi = scope_cv_disp[1][in][c];
        if (lo > hi) { prev_y = -1; continue; } // column not captured yet
        int ytop = Y(hi), ybot = Y(lo);
        if (prev_y >= 0) { // join to the previous column
          ytop = min(ytop, prev_y);
          ybot = max(ybot, prev_y);
        }
        gfxLine(x0 + px, ytop, x0 + px, ybot);
        prev_y = Y((lo + hi) / 2);
      }
      continue;
    }

    const int bit = sig - 1; // C1-C4 = bits 0-3, A-H = bits 4-11
    // Roll draws the ring oldest-first, so the newest column is on the right.
    // Sweep draws it in place, leaving a gap just ahead of the write point.
    const int first = scope_mode == SCOPE_ROLL ? scope_head : 0;
    // The page draws the columns after the labels (x = 14-127); the
    // screensaver spreads them over the full width instead.
    const int x0 = interactive ? 14 : 0;
    const int width = interactive ? kScopeCols : 128;
    bool prev = scope_disp[first] >> bit & 1;
    for (int px = 0; px < width; ++px) {
      const int c = interactive ? px : px * kScopeCols / 128;
      const bool hi = scope_disp[(first + c) % kScopeCols] >> bit & 1;
      const int x = x0 + px;
      if (scope_mode == SCOPE_SWEEP &&
          (c - scope_head + kScopeCols) % kScopeCols < kSweepGap) {
        prev = hi; // no line across the gap
        continue;
      }
      gfxPixel(x, hi ? y + 1 : y + h - 2);
      if (hi != prev) gfxLine(x, y + 1, x, y + h - 2);
      prev = hi;
    }
  }

  if (!interactive) return;
  if (scope_cursor == SCOPE_MODE) DrawFieldCursor(0, 7, 25);
  else if (scope_cursor == SCOPE_TRIG) DrawFieldCursor(29, 7, 19);
  else if (scope_cursor == SCOPE_SPAN) DrawFieldCursor(53, 7, 31);
  else if (scope_cursor == SCOPE_CAPTURE) DrawFieldCursor(87, 7, 13);
}

// Keeps the selected trace on screen when zoomed in, scrolling the window.
FLASHMEM void XandNor::ScopeFollowCursor() {
  using namespace XandNorDefs;
  const int visible = kScopeTraces >> scope_zoom;
  scope_win = constrain(scope_win, 0, kScopeTraces - visible);
  if (scope_cursor < SCOPE_TRACE0) return;
  const int t = scope_cursor - SCOPE_TRACE0;
  if (t < scope_win) scope_win = t;
  if (t >= scope_win + visible) scope_win = t - visible + 1;
}

// Scroll bar: a dotted track with a solid thumb.
FLASHMEM void XandNor::DrawScrollBar(int scroll_pos, int rows) const {
  using namespace XandNorDefs;
  gfxDottedLine(126, 12, 126, 56);
  const int thumb = 45 * kVisibleRows / rows;
  const int ty = 12 + (45 - thumb) * scroll_pos / (rows - kVisibleRows);
  gfxRect(125, ty, 3, thumb);
}

// Outputs page: a row per output with the settings its function needs.
// CNT/BIN: step size. ENV: mode, attack, decay. Others have none.
static constexpr int OutputFields(uint8_t fn) {
  return fn == XandNorDefs::FN_ENV ? 3 : (fn == XandNorDefs::FN_CNT || fn == XandNorDefs::FN_BIN) ? 1 : 0;
}
// Cursor stops per row: rows with no settings still get one (on the "-"), so
// the cursor can reach, and scroll to, every output.
static constexpr int OutputStops(uint8_t fn) { return OutputFields(fn) ? OutputFields(fn) : 1; }
static constexpr XandNorDefs::FieldPos OutputFieldPos(uint8_t fn, int f) {
  return fn == XandNorDefs::FN_ENV
    ? (f == 0 ? XandNorDefs::FieldPos{0, 36, 18} : f == 1 ? XandNorDefs::FieldPos{0, 58, 30}
                                                         : XandNorDefs::FieldPos{0, 92, 30})
    : XandNorDefs::FieldPos{0, 60, 24};
}

FLASHMEM void XandNor::DrawOutputs() const {
  using namespace XandNorDefs;
  for (int o = out_scroll; o < out_scroll + kVisibleRows && o < kNumOutputs; ++o) {
    const int y = 13 + (o - out_scroll) * 9;
    const char name[2] = { char('A' + o), '\0' };
    gfxPrint(1, y, name);
    const uint8_t fn = routes[o].fn;
    gfxPrint(10, y, kFunctionNames[fn]);
    if (fn == FN_CNT || fn == FN_BIN) {
      gfxPrint(60, y, kStepNames[out_step[o]]);
    } else if (fn == FN_ENV) {
      gfxPrint(36, y, kEnvModeNames[env_mode[o]]);
      gfxPos(58, y);
      PrintMicros(kLengthUs[env_attack[o] - 1]);
      gfxPos(92, y);
      PrintMicros(kLengthUs[env_decay[o] - 1]);
    } else {
      gfxPrint(40, y, "-");
    }
  }
  DrawScrollBar(out_scroll, kNumOutputs);
  const int o = out_cursor / 3, f = out_cursor % 3;
  const uint8_t fn = routes[o].fn;
  if (o >= out_scroll && o < out_scroll + kVisibleRows) {
    const int y = 13 + (o - out_scroll) * 9 + 8;
    if (f < OutputFields(fn)) {
      const FieldPos fp = OutputFieldPos(fn, f);
      DrawFieldCursor(fp.x - 1, y, fp.w);
    } else {
      DrawFieldCursor(39, y, 7); // the "-" of a row with no settings
    }
  }
}

FLASHMEM void XandNor::DrawMod() const {
  using namespace XandNorDefs;
  for (int r = mod_scroll; r < mod_scroll + kVisibleRows && r < kModRows; ++r) {
    const int y = 13 + (r - mod_scroll) * 9;
    if (r < 8) {
      // "A  C 3  100%  > XOR": output, function CV, depth, function in use
      const char name[2] = { char('A' + r), '\0' };
      gfxPrint(1, y, name);
      gfxPrint(14, y, fn_cv[r].InputName());
      PrintDepth(40, y, fn_cv[r]);
      if (fn_cv[r].enabled()) DrawArrow(74, y);
      gfxPrint(82, y, kFunctionNames[live_fn[r]]);
    } else if (r == 8) {
      gfxPrint(1, y, "Preset:");
      gfxPrint(52, y, preset_cv.InputName());
      PrintDepth(88, y, preset_cv);
    } else if (r == 9) {
      gfxPrint(1, y, "Load on:");
      gfxPrint(52, y, preset_trig.InputName());
      // With no preset CV, each trigger steps to the next used slot.
      if (!preset_cv.enabled() && (preset_trig.Pack() & 0xff)) gfxPrint(76, y, "next");
    } else {
      gfxPrint(1, y, "Audio:"); // preset loads include the audio setup
      gfxPrint(52, y, preset_audio ? "On" : "Off");
    }
  }
  DrawScrollBar(mod_scroll, kModRows);
  const FieldPos f = ModFieldPos(mod_cursor);
  DrawFieldCursor(f.x - 1, 13 + (f.row - mod_scroll) * 9 + 8, f.w);
}

// Draws "FN a b" from x, e.g. "XOR 3 4" for CH3 XOR CH4. Only channel numbers
// fit in half the screen; the function gets a 4-character column so the
// operands line up down the page.
FLASHMEM void XandNor::PrintRoute(int x, int y, const XandNorDefs::Route &r) const {
  using namespace XandNorDefs;
  gfxPrint(x, y, kFunctionNames[r.fn]);
  if (UsesA(r.fn)) gfxPrint(x + 28, y, r.a + 1);
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
  gfxHeader("(X)AND(N)OR");
  // "Bank# 7", as in Quadrants' preset selector: a fixed position with room
  // for two digits, so the text doesn't move between 9 and 10.
  gfxPrint(128 - 6 * 8 - 1, 1, "Bank# ");
  gfxPrint(bank);
  for (int i = 0; i < kNumSlots; ++i) {
    const int x = 4 + (i % 4) * 31;
    const int y = 15 + (i / 4) * 17;
    gfxFrame(x, y, 26, 14);
    if (i == active_slot) gfxFrame(x + 1, y + 1, 24, 12);
    gfxPrint(x + 6, y + 3, i + 1);
    if (slot_used & (1 << i)) gfxRect(x + 16, y + 5, 3, 3);
    if (i == slot_cursor) gfxInvert(x, y, 26, 14);
  }
  if (int32_t(message_until - millis()) > 0) {
    if (message_badge) { // "3 saved" left, "[SD]" right
      gfxPrint(1, 52, message);
      gfxPrint(127 - 6 * (int)strlen(message_badge), 52, message_badge);
    } else {
      gfxPrint(64 - 3 * (int)strlen(message), 52, message);
    }
  } else {
    gfxPrint(1, 52, kPresetHint);
    const char *where = SDcard_Ready ? "[SD]" : "[Int]"; // where banks are saved
    gfxPrint(127 - 6 * (int)strlen(where), 52, where);
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
    message_until = 0;
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
  if (preset_mode) { // left encoder: bank (loads it at once)
    const int b = constrain(bank + direction, 0, kNumBanks - 1);
    if (b != bank) SelectBank(b);
    return;
  }
  if (page == PAGE_ROUTING) {
    // Step through fields, skipping the second channel of OUT and NOT.
    const int last = kNumOutputs * ROUTE_FIELDS - 1;
    int c = route_cursor;
    do {
      c += (direction > 0) ? 1 : -1;
    } while (c >= 0 && c <= last &&
             ((c % ROUTE_FIELDS == ROUTE_B && !TwoInputs(routes[c / ROUTE_FIELDS].fn)) ||
              (c % ROUTE_FIELDS == ROUTE_A && !UsesA(routes[c / ROUTE_FIELDS].fn))));
    if (c >= 0 && c <= last) route_cursor = c;
    return;
  }
  if (page == PAGE_SCOPE) {
    scope_cursor = constrain(scope_cursor + direction, 0, SCOPE_FIELDS - 1);
    ScopeFollowCursor();
    return;
  }
  if (page == PAGE_OUTPUTS) {
    // Step through the fields that exist, skipping outputs with none.
    int c = out_cursor;
    do {
      c += (direction > 0) ? 1 : -1;
    } while (c >= 0 && c < kNumOutputs * 3 && c % 3 >= OutputStops(routes[c / 3].fn));
    if (c >= 0 && c < kNumOutputs * 3) out_cursor = c;
    const int row = out_cursor / 3;
    if (row < out_scroll) out_scroll = row;
    if (row >= out_scroll + kVisibleRows) out_scroll = row - kVisibleRows + 1;
    return;
  }
  if (page == PAGE_CVMOD) {
    mod_cursor = constrain(mod_cursor + direction, 0, MOD_COUNT - 1);
    const int row = ModFieldPos(mod_cursor).row;
    if (row < mod_scroll) mod_scroll = row;
    if (row >= mod_scroll + kVisibleRows) mod_scroll = row - kVisibleRows + 1;
    return;
  }
  if (page < PAGE_CHANNEL_A || page > PAGE_CHANNEL_D) return;
  cursor = constrain(cursor + direction, 0, FIELD_COUNT - 1);
  const int row = kFieldPos[cursor].row;
  if (row < scroll) scroll = row;
  if (row >= scroll + kVisibleRows) scroll = row - kVisibleRows + 1;
}

FLASHMEM static void AdjustDepth(CVInputMap &m, int direction) {
  m.attenuversion = constrain(m.attenuversion + direction, -127, 127);
}

FLASHMEM void XandNor::OnRightEncoderMove(int direction) {
  using namespace XandNorDefs;
  if (page == PAGE_SCOPE && !preset_mode) {
    if (scope_cursor == SCOPE_MODE) {
      scope_mode = constrain(scope_mode + direction, 0, SCOPE_MODES - 1);
      scope_col = -1;
    } else if (scope_cursor == SCOPE_TRIG) {
      scope_trig.ChangeSource(direction);
      scope_period = 0;
    } else if (scope_cursor == SCOPE_SPAN) {
      scope_span = constrain(scope_span + direction, 0, kScopeSpans - 1);
    } else if (scope_cursor == SCOPE_CAPTURE) {
      scope_peak = direction < 0; // Peak first, Sample to the right
    } else {
      uint8_t &t = scope_traces[scope_cursor - SCOPE_TRACE0];
      t = constrain(t + direction, 0, kScopeSignals - 1);
    }
    return;
  }
  if (page == PAGE_OUTPUTS && !preset_mode) {
    const int o = out_cursor / 3, f = out_cursor % 3;
    const uint8_t fn = routes[o].fn;
    if (f >= OutputFields(fn)) return;
    if (fn != FN_ENV) {
      out_step[o] = constrain(out_step[o] + direction, 0, CVSTEP_COUNT - 1);
    } else if (f == 0) {
      env_mode[o] = constrain(env_mode[o] + direction, 0, EMODE_COUNT - 1);
    } else if (f == 1) {
      env_attack[o] = constrain(env_attack[o] + direction, 1, kNumLengths);
    } else {
      env_decay[o] = constrain(env_decay[o] + direction, 1, kNumLengths);
    }
    return;
  }
  if (page == PAGE_CVMOD && !preset_mode) {
    if (mod_cursor < MOD_PRESET_CV) {
      CVInputMap &m = fn_cv[mod_cursor / 2];
      if (mod_cursor % 2) AdjustDepth(m, direction);
      else m.ChangeSource(direction);
    } else if (mod_cursor == MOD_PRESET_CV) {
      preset_cv.ChangeSource(direction);
    } else if (mod_cursor == MOD_PRESET_DEPTH) {
      AdjustDepth(preset_cv, direction);
    } else if (mod_cursor == MOD_PRESET_TRIG) {
      preset_trig.ChangeSource(direction);
    } else {
      preset_audio = direction > 0;
    }
    return;
  }
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
    case FIELD_DIV:
      div_steps[ch] = DivIndexToSteps(
        constrain(DivStepsToIndex(div_steps[ch]) + direction, kDivIndexMin, kDivIndexMax));
      break;
    case FIELD_LENGTH:
      if (clock_length[ch]) {
        clock_pct[ch] = constrain(clock_pct[ch] + direction, 1, 100);
      } else if (level[ch]) {
        if (direction > 0) level[ch] = false; // back up to Thru
      } else if (length[ch] == kLengthThru && direction < 0) {
        level[ch] = true;                     // Level sits below Thru
      } else {
        length[ch] = constrain(length[ch] + direction, kLengthThru, kLengthInfinite);
      }
      break;
    case FIELD_LEN_MODE:
      if (level[ch]) threshold_dv[ch] = constrain(threshold_dv[ch] + direction, 1, kMaxThresholdDv);
      else clock_length[ch] = direction > 0;
      break;
    case FIELD_DELAY:
      delay[ch] = constrain(delay[ch] + direction, 0, kNumLengths);
      break;
    case FIELD_DELAY_CV:   delay_cv[ch].ChangeSource(direction); break;
    case FIELD_DELAY_DEPTH: AdjustDepth(delay_cv[ch], direction); break;
    case FIELD_LEN_CV:   length_cv[ch].ChangeSource(direction); break;
    case FIELD_LEN_DEPTH: AdjustDepth(length_cv[ch], direction); break;
    case FIELD_PROB:
      prob[ch] = constrain(prob[ch] + direction, 0, 100);
      break;
    case FIELD_PROB_CV:  prob_cv[ch].ChangeSource(direction); break;
    case FIELD_PROB_DEPTH: AdjustDepth(prob_cv[ch], direction); break;
    case FIELD_RATCHET:
      ratchets[ch] = constrain(ratchets[ch] + direction, 1, kMaxRatchets);
      break;
    case FIELD_RAT_CV:   ratchet_cv[ch].ChangeSource(direction); break;
    case FIELD_RAT_DEPTH: AdjustDepth(ratchet_cv[ch], direction); break;
    case FIELD_DIV_CV:   div_cv[ch].ChangeSource(direction); break;
    case FIELD_DIV_DEPTH: AdjustDepth(div_cv[ch], direction); break;
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
  stream_buffer.Write(bank);
  stream_buffer.Write(working);
  return stream_buffer.written();
}

FLASHMEM size_t XandNor::RestoreAppData(util::StreamBufferReader &stream_buffer) {
  using namespace XandNorDefs;
  const uint8_t version = stream_buffer.Read<uint8_t>();

  if (version == kStorageVersion) {
    const uint8_t b = stream_buffer.Read<uint8_t>();
    PackedPatch working;
    stream_buffer.Read(working);
    if (stream_buffer.underflow()) return stream_buffer.read();
    SelectBank(constrain(b, 0, kNumBanks - 1));
    UnpackPatch(working, true);
    return stream_buffer.read();
  }

  if (version == kStorageVersionV2) {
    // Migrate: the v2 slots become bank 00, written out by Loop().
    const uint8_t used = stream_buffer.Read<uint8_t>();
    uint8_t raw[1 + kNumSlots][kPatchV2Size];
    for (auto &r : raw) stream_buffer.Read(r);
    if (stream_buffer.underflow()) return stream_buffer.read();
    PackedPatch working;
    memset(&working, 0, sizeof(working));
    memcpy(&working, raw[0], kPatchV2Size);
    memset(slots, 0, sizeof(slots));
    for (int i = 0; i < kNumSlots; ++i) memcpy(&slots[i], raw[1 + i], kPatchV2Size);
    bank = 0;
    slot_used = used;
    bank_dirty = true;
    UnpackPatch(working, true);
  }
  return stream_buffer.read();
}

FLASHMEM void XandNor::HandleAppEvent(OC::AppEvent event) {
  if (event == OC::APP_EVENT_RESUME) Resume();
}

// Applies a preset load requested by the preset trigger, outside the interrupt.
FLASHMEM void XandNor::Loop() {
#ifdef XANDNOR_AUDIO
  audio_app.mainloop();
#endif
  if (tidy_pending) { // once per boot, before any bank writes
    tidy_pending = false;
    TidyInternalBanks();
  }
  if (bank_dirty) { // migrated v2 slots: write them out as bank 00
    bank_dirty = false;
    WriteBank();
  }
  int slot = pending_slot;
  if (slot == -1) return;
  pending_slot = -1;
  if (slot == -2) { // next used slot after the active one
    if (!slot_used) return;
    slot = active_slot;
    do { slot = (slot + 1) % XandNorDefs::kNumSlots; } while (!(slot_used & (1 << slot)));
  }
  if (slot_used & (1 << slot)) LoadSlot(slot); // an empty slot is ignored
}

FLASHMEM void XandNor::DrawMenu() const { BaseView(); }

// The scope keeps sampling on every page, so it doubles as the screensaver.
FLASHMEM void XandNor::DrawScreensaver() const { DrawScope(false); }

void XandNor::DrawDebugInfo() const {}

FLASHMEM void XandNor::HandleButtonEvent(const UI::Event &event) {
#ifdef XANDNOR_AUDIO
  // Audio page: every button goes to the audio engine, as in Quadrants. A or B
  // pressed at its top level hands back: A to the previous page, B to the next.
  if (page == XandNorDefs::PAGE_AUDIO && !preset_mode) {
    // The audio engine only uses short presses, so long presses keep our
    // shortcuts here: A or Y for presets, X for Routing.
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) {
      if (event.control == OC::CONTROL_BUTTON_A || event.control == OC::CONTROL_BUTTON_Y)
        OnUpButtonLongPress();
      if (event.control == OC::CONTROL_BUTTON_X) ToggleRouting();
      return;
    }
    if (event.control == OC::CONTROL_BUTTON_L || event.control == OC::CONTROL_BUTTON_R) {
      audio_app.HandleEncoderButtonEvent(event);
    } else if (audio_app.HandleButtonEvent(event)) {
      if (event.control == OC::CONTROL_BUTTON_A) OnUpButtonPress();
      if (event.control == OC::CONTROL_BUTTON_B) OnDownButtonPress();
    }
    return;
  }
#endif
  if (event.control == OC::CONTROL_BUTTON_UP) {
    if (event.type == UI::EVENT_BUTTON_LONG_PRESS) OnUpButtonLongPress();
    if (event.type == UI::EVENT_BUTTON_PRESS) OnUpButtonPress();
  }
  if (event.control == OC::CONTROL_BUTTON_DOWN && event.type == UI::EVENT_BUTTON_PRESS)
    OnDownButtonPress();
  // Scope: right encoder press freezes or resumes the display.
  if (page == XandNorDefs::PAGE_SCOPE && !preset_mode &&
      event.control == OC::CONTROL_BUTTON_R && event.type == UI::EVENT_BUTTON_PRESS)
    scope_hold = !scope_hold;
  // Scope: left encoder press zooms in, 8 -> 4 -> 2 -> 1 traces, then back to 8.
  if (page == XandNorDefs::PAGE_SCOPE && !preset_mode &&
      event.control == OC::CONTROL_BUTTON_L && event.type == UI::EVENT_BUTTON_PRESS) {
    scope_zoom = (scope_zoom + 1) % 4;
    ScopeFollowCursor();
  }
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
#ifdef XANDNOR_AUDIO
  if (page == XandNorDefs::PAGE_AUDIO && !preset_mode) {
    audio_app.HandleEncoderEvent(event);
    return;
  }
#endif
  if (event.control == OC::CONTROL_ENCODER_L) OnLeftEncoderMove(event.value);
  if (event.control == OC::CONTROL_ENCODER_R) OnRightEncoderMove(event.value);
}
