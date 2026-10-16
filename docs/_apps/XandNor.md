---
layout: default
---
# (X)AND(N)OR

> **Hardware: Teensy 4.1 [O.R.N.8](https://www.tlm-audio.com/product-page/oc-4-1-phazerville) required.** To get the best out of it, (X)AND(N)OR is built around the O.R.N.8's more generous I/O: eight outputs (A–H) for its gates, logic and CV functions, and eight CV inputs (1–8), so every channel setting can have its own modulation. It also uses the O.R.N.8's X and Y buttons, its audio applets and its microSD slot for preset banks. It is included only in Teensy 4.1 (T41) builds; 4-output Teensy 4.0 and 3.2 modules have neither the I/O nor the save space for it.

*(X)AND(N)OR* turns triggers into gates and combines them. Four gate channels (CH1–CH4) each take a trigger from any input and produce a gate from 1 ms to 10 s long, shaped by a divider, delay, probability and ratchets. Eight outputs (A–H) each carry a channel's gate, a logic combination of two channels, a toggle or latch, a stepped CV built from all four channels, or an envelope. Almost every setting can be modulated by CV.

The app also hosts the O.R.N.8's audio applets, has a built-in logic-analyser scope that doubles as its screensaver, and stores 100 banks of 8 presets on the SD card.

Panel labels used here: outputs **A–H**, CV inputs **1–8** (shown on screen as CV 1–CV 8), trigger inputs **TR 1–TR 4**. The Up and Down buttons are labelled **A** and **B** on the O.R.N.8.

## Controls

| Control | Action |
| --- | --- |
| **A** / **B** | Previous / next page |
| Left encoder turn | Move the cursor between fields (pages with long lists scroll to follow) |
| Right encoder turn | Change the selected value directly; there is no press to start editing |
| Long-press **A**, or **Y** | Open or close the preset overlay |
| **X** | Jump to the Routing page; press again to go back |
| **Z** | Not used by the app: it remains the platform's menu modifier (Z + encoder press) |

The field cursor is a steady underline. On the Audio page the audio engine takes all short presses; see [Audio](#audio).

Pages, in order: **Overview**, **CH1–CH4**, **Routing**, **Outputs**, **CV Mod**, **Audio**, **Scope**. The list wraps, so **A** from Overview goes straight to Scope.

## Overview

Boxes for CH1–CH4 and outputs A–H fill while they are high, and the last preset saved or loaded is shown underneath. A small box in the header of each channel page shows that channel's gate in the same way.

## Channel pages (CH1–CH4)

Each channel turns triggers into gates. The page is a scrolling list:

| Row | Setting |
| --- | --- |
| **Source** | The trigger: TR 1–TR 4, a CV input (**CV 1**–**CV 8**), another channel, an output (**OutA**–**OutH**), a virtual output (**VrtI**–**VrtP**), or MIDI. The channels come straight after CV 8 in the list: **CH1>** fires when CH1's gate starts, **CH1<** when it ends, then CH2>, CH2< … CH4<. Next to it, the **divider**: turning right goes x1, x2, x3 … (multiply), turning left /2, /3 … (divide) |
| **Length** | Gate length from 1 ms to 10 s, in ten steps per decade. Below 1 ms: **Thru** (the output follows the source while it is high) and **Level** (see below). Above 10 s: **Inf** (high until the next trigger). In Clock mode this row is a percentage instead |
| **CV** | Length CV input and depth |
| **Len by** | **Time** or **Clock**: in Clock mode Length is 1–100% of the measured time between triggers (after the divider), so gates keep their shape when the tempo changes. Until two triggers have been measured, the Time setting applies. In Level mode this row is **Thresh** |
| **Delay** | Off, or 1 ms to 10 s between the trigger and the gate. Up to four triggers can be waiting at once, so a delay longer than the clock period still fires every trigger |
| **CV** | Delay CV input and depth |
| **Prob** | Chance that a trigger fires, 0–100%. A skipped trigger starts nothing and leaves a running gate alone |
| **CV** | Probability CV input and depth |
| **Ratchet** | Splits each gate into 1–8 pulses at 50% duty; each pulse is at least 1 ms long |
| **CV** | Ratchet CV input and depth |
| **Div CV** | Divider CV input and depth |
| **CV mode** | **Sample**: CV is read when the trigger arrives and kept for the whole gate. **Track**: CV is followed while the gate runs (length and ratchets change mid-gate) |

When a CV is patched, its row shows an arrow followed by the value currently in use.

A trigger that arrives while the gate is still high briefly drops the gate (240 µs) before restarting it, so downstream modules see a new edge.

**Channel sources.** Chaining on a channel's end (CH2 on **CH1<**, CH3 on **CH2<** …) makes sequential pulses from one trigger, with no patching; each link adds about 0.06 ms. A channel set to its own end (CH1 on **CH1<**) starts itself and retriggers as each gate ends: a free-running pulse source whose period is its length plus any delay. The divider, delay, probability and ratchets apply to channel sources too. In Thru or Level, the output follows the source channel's gate (inverted for **<**).

**Level mode.** The channel is high while its source is above the threshold (0.1–6.3 V, default 1.2 V), with 0.2 V of hysteresis so it switches cleanly. With a CV input as the source, an LFO becomes a gate whose width follows the threshold. Delay, probability and ratchets don't apply in Level or Thru.

**CV sources.** Every CV row offers the CV inputs (**CV 1**–**CV 8**) and also the outputs: **OutA**–**OutH** are (X)AND(N)OR's own outputs, read back one tick later, so one output can modulate a channel with no patch cable (for example an **ENV** on H as CH1's Length CV, or a **CNT** output as another channel's Prob CV). **VrtI**–**VrtP** are the platform's virtual outputs, shared with Quadrants: Quadrants' clock writes some of them, which doesn't run here, and some audio applets write CV to them (TuneTracker's detected pitch, for example), which does.

### CV scaling

Each CV row has a depth (the platform's attenuverter, −448% to +448%; 100% is the default). At 100%:

| CV target | +5 V gives |
| --- | --- |
| Length, Delay | +40 steps, the whole 1 ms – 10 s range (8 steps per volt) |
| Length in Clock mode | +100% |
| Probability | +100% |
| Ratchet | +7 pulses |
| Divider | +10 steps towards multiply (2 steps per volt) |

Negative depth, or negative voltage, moves the other way. Length is held within 1 ms – 10 s; CV can open a delay from Off and close it again.

## Routing

Each output takes a function and one or two channels. Turn the left encoder through function, first channel and second channel; the cursor skips fields a function doesn't use. The output letter is highlighted while the output is high.

| Function | Reads | Output |
| --- | --- | --- |
| OUT / NOT | one channel | its gate, or the inverse |
| AND, NAND, OR, NOR, XOR, XNOR | two channels | the logic combination |
| TOGL | one channel | flips on each rising edge (divides by two) |
| LTCH | two channels | set by the first, reset by the second; simultaneous edges are ignored |
| CNT | all four | stepped CV: how many channels are high (0–4) |
| BIN | all four | stepped CV: CH1 + 2×CH2 + 4×CH3 + 8×CH4 (0–15) |
| ENV | one channel | 0–5 V envelope |

### Default patch

A fresh install and empty preset slots start here. All four channels take TR 1, with lengths 10, 50, 100 and 250 ms, so one clock fans out into nested gates and the slices between them:

| Output | Function | Result on each clock |
| --- | --- | --- |
| A–D | OUT 1–4 | 10, 50, 100 and 250 ms gates |
| E | XOR 1 2 | 10–50 ms |
| F | XOR 2 3 | 50–100 ms |
| G | XOR 3 4 | 100–250 ms |
| H | NOT 4 | from 250 ms until the next clock |

## Outputs

Settings for the functions that need them, one row per output; rows with none show "–".

- **CNT / BIN:** step size **Semi** (a semitone, for pitch), **1/3V** or **1V**.
- **ENV:** mode **AD** (each rising edge runs attack then decay, whatever the gate length) or **ASR** (attack, hold while the gate is high, then release), with attack (default 2 ms) and decay or release (default 200 ms) on the 1 ms – 10 s scale.

## CV Mod

Voltage control of each output's logic function, and of preset loading. The page scrolls: rows **A–H** first, then **Preset**, **Load on** and **Audio**.

### Function by CV (outputs A–H)

Each output row reads, left to right: the output letter, its function **CV source**, the **depth**, and after the arrow the **function in use** right now.

```
A  CV 3  100%  > NAND
```

The CV moves the function on from the one set on the Routing page, through the eight logic functions in this order:

**OUT → NOT → AND → NAND → OR → NOR → XOR → XNOR**

- At 100% depth each step takes 0.625 V, so +5 V covers all eight. Negative voltage, or negative depth, moves back towards OUT. The result stops at either end rather than wrapping.
- The function changes on the next tick (60 µs), so audio-rate modulation is possible.
- The output keeps the channels set on the Routing page. A one-channel function (OUT, NOT) reads the first; a two-channel function reads both, so set both channels on Routing if the CV can reach the two-input functions.
- Outputs set to **TOGL**, **LTCH**, **CNT**, **BIN** or **ENV** aren't affected: they keep state, so switching them by CV wouldn't behave predictably.
- Several outputs can share one CV source, each with its own depth.

### CV sources

Every CV row in the app (here and on the channel pages) offers the same sources:

| Source | What it is |
| --- | --- |
| **CV 1**–**CV 8** | The CV input jacks |
| **OutA**–**OutH** | (X)AND(N)OR's own outputs, read back one tick later, so one output can modulate another with no patch cable. Gate functions give 0 or 5 V, **ENV** 0–5 V, **CNT** / **BIN** their stepped voltage |
| **VrtI**–**VrtP** | The platform's virtual outputs, shared with Quadrants. Quadrants' clock writes some of them but doesn't run here; some audio applets write CV to them (TuneTracker's detected pitch, for example) |
| **M 1**… | The platform's MIDI CV mappings, set up in its MIDI settings |
| **G*** | Noise: a new random value on every tick |
| **–** | Off |

**Depth for gate sources.** A gate output is 0 or 5 V, so at 100% it jumps all the way to XNOR. To step one function per gate, set the depth to about **13%**; two functions need about **25%**. The depth readout shows the percentage actually applied.

### Examples

- **Flip a logic output with a gate:** set E to **AND 1 2** on Routing, then give E the source **OutH** at about 13%. While H is high, E becomes **NAND**, its exact inverse.
- **A function sequencer:** give output F a slow stepped random voltage, or a **BIN** output at **1/3V** steps from this app, at 100% depth. F then walks through the logic functions as the voltage changes, while the Overview and Scope show the result.
- **Feedback:** an output modulating its own function (A with source **OutA**) can lock up or oscillate depending on its channels. It's unpredictable, which can be the point.

### Preset by CV and audio

- **Preset:** a CV input and depth that pick slot 1–8 of the current bank (0–5 V, about 0.6 V per slot).
- **Load on:** the trigger that loads the slot the Preset CV points to. With no Preset CV set, each trigger steps to the next saved slot ("next").
- **Audio:** whether loading a preset also loads its audio setup (default On).

These settings are global: loading a slot doesn't change them.

## Audio

The O.R.N.8's audio applets, exactly as in Quadrants' audio setup. While (X)AND(N)OR is running, the audio applets follow CV and triggers here too, whichever page you are on.

On this page all short presses of A, B, X and Y and both encoders go to the audio engine. Pressing **A** or **B** at its top level moves to the previous or next page. Long-press **A** or **Y** for presets, or **X** for Routing.

The audio setup is shared with Quadrants: changes made in either app appear in the other.

## Scope

A logic analyser of the app's own signals, with no patching. The status line shows mode, trigger, timebase, capture and HOLD; below it are up to eight traces.

| Setting | Options |
| --- | --- |
| Mode | **Swp** (default): the picture stays still and is redrawn in place as new data arrives. **Roll**: scrolls, newest on the right. **Trig**: each sweep starts at the trigger and is drawn as it is captured |
| Trigger | Any input or output (default TR 1), or none for free-running |
| Timebase | **Auto**, or 5 ms to 10 s per screen. Auto fits two trigger periods in Swp and Trig, and is a fixed 2 s in Roll so the scroll speed never depends on the clock |
| Capture | **Pk** (default): each column records everything that happened during it, so no pulse is missed. **Sm**: each column takes a single sample, as the Scope applet does |
| Traces | Each of the eight shows **C1–C4** (channels), **A–H** (outputs) or **V1–V8** (CV inputs, drawn from −5 V to +5 V), or "--". Default: C1–C4 and E–H. In Pk mode a CV trace shows each column's full range, so fast modulation appears as a band |

- **Left encoder press:** zoom, 8 → 4 → 2 → 1 traces, each taller as fewer share the screen. Moving the cursor through the traces scrolls the view.
- **Right encoder press:** hold (freeze) the display.
- Changing mode, trigger, timebase or capture clears the screen and starts afresh.

The selected trace's label is inverted. Scope settings aren't saved with presets.

**Screensaver.** The scope keeps capturing on every page and is the app's screensaver: the same mode and zoom, without labels or status line, across the full width. How long the module waits is the platform's **Screen Blank** setting in the Setup/About calibration routine.

## Presets and banks

Long-press **A** or press **Y** to open the preset overlay: 100 banks of 8 slots.

| Control | Action |
| --- | --- |
| Left encoder | Bank 0–99; the bank loads at once |
| Right encoder | Slot 1–8 |
| **A** | Save the current patch to the slot |
| **B** | Load the slot (an empty slot loads the default patch) |
| Encoder press, long-press **A** or **Y** | Close the overlay |

Slots holding a patch carry a dot; the active slot is framed twice. The footer badge shows where banks are stored, **[SD]** or **[Int]**, and saving or changing bank confirms it, e.g. "Saved [SD]" or "3 saved [SD]".

- **Storage.** Each bank is a file, `XNBANK00.DAT` to `XNBANK99.DAT`, with a companion `XNAUDnn.DAT` holding the slots' audio setups. Files go to the microSD card when one is ready, otherwise to the Teensy's internal flash. Every save is read back and checked; if the card fails, the save falls back to internal flash. Bank files left in internal flash move to the card automatically when one is present. A FAT32 (or exFAT) card is recommended: internal flash is smaller and can stutter while saving.
- **Audio in presets.** Saving a slot also stores the audio applet setup; loading applies it unless **Audio** is Off on the CV Mod page. Slots saved before this existed have no audio setup and leave audio alone.
- **The working patch.** Your current settings and bank number are kept with the platform's normal app save: long-press the right encoder on (X)AND(N)OR in the app menu. Preset slots are saved to their files at once when you press A.

## Examples

### Using BIN

BIN treats each channel as one binary digit and outputs the total of the channels that are high:

| Channel | Worth |
| --- | --- |
| CH1 | 1 step |
| CH2 | 2 steps |
| CH3 | 4 steps |
| CH4 | 8 steps |

That gives 0 to 15 steps. Set the step size on the Outputs page: **Semi** (a semitone, for pitch), **1/3V** (0–5 V in total, for modulation) or **1V** (15 V in total, more than the outputs can produce).

#### A 16-step staircase from one clock

Clock TR 1 and set the four channels to divide it by powers of two, each with a gate half its period long:

| Channel | Source | Divider | Len by | Length |
| --- | --- | --- | --- | --- |
| CH1 | TR 1 | /2 | Clock | 50% |
| CH2 | TR 1 | /4 | Clock | 50% |
| CH3 | TR 1 | /8 | Clock | 50% |
| CH4 | TR 1 | /16 | Clock | 50% |

Set an output to **BIN** with step **1/3V**. The channels now count like the digits of a binary counter, so the output steps down 15, 14, 13 … 0, one step per clock, and repeats every 16 clocks: a descending staircase from 5 V to 0 V. On the Scope page, C1–C4 show the counter pattern, each half the speed of the one before.

Use it as a stepped sweep for a filter cutoff, or with **Semi** steps into a VCO for a descending chromatic run. Clock-relative length needs two clocks to measure the tempo, so the very first step may be off.

#### A generative melody

Odd dividers make the channels drift in and out of phase with each other:

| Channel | Source | Divider | Length |
| --- | --- | --- | --- |
| CH1 | TR 1 | x1 | 100 ms |
| CH2 | TR 1 | /3 | 300 ms |
| CH3 | TR 1 | /4 | 400 ms |
| CH4 | TR 1 | /5 | 600 ms |

Set an output to **BIN** with **Semi** steps, and patch it through a quantiser set to a scale into a VCO's 1 V/oct input. Use output E (XOR of CH1 and CH2) as the trigger for your envelope, or set another output to **ENV** in AD mode on CH1 for a percussive VCA envelope.

Each clock produces a note from the current combination of high channels, and the pattern repeats only every 60 clocks. Adding **Prob** on CH1, or CV on a divider, makes it evolve.

### Ratchet rolls on demand

Set CH2 to TR 1 with a length of about 80% of the clock period (or **Len by: Clock** at 80%), **Ratchet** 1, and patch a slow LFO or a stepped random voltage into CV 2 with CH2's Ratchet **CV** set to input 2. As the voltage rises, each gate splits into 2, 3 … up to 8 pulses: drum rolls that come and go. With **CV mode: Track**, the roll changes speed within a single gate.

### Off-beats and swing from one clock

With the default patch running, set CH2 to **x1**, **Delay** to half the clock period, and its Length to 10 ms: output B now fires on the off-beat. Patch a slow LFO into CH2's Delay **CV** for a moving swing feel, and route **E** to **OR 1 2** for a combined on-and-off-beat pattern.
