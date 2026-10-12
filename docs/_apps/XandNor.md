---
layout: default
---
# (X)AND(N)OR

*(X)AND(N)OR* turns triggers into gates and combines them. Four gate channels (CH1–CH4) each take a trigger from any input and produce a gate from 1 ms to 10 s long. Eight outputs (A–H) each carry one channel's gate, a logic combination of two channels, or one of several stateful and CV functions.

It is built for Teensy 4.1 (ORN8) hardware only: outputs A–H, CV inputs 1–8, trigger inputs T1–T4. On the ORN8 panel the Up and Down buttons are labelled **A** and **B**.

*This manual is a work in progress; more sections will follow.*

### Pages

Press **A** / **B** to move between pages; the left encoder moves the cursor, the right encoder changes the selected value.

| Page | What it does |
| --- | --- |
| **Overview** | Live state of CH1–CH4 and outputs A–H |
| **CH1–CH4** | Per channel: trigger source and divider, length (or Thru / Level / Inf), delay, probability, ratchet, each with optional CV, and the CV mode (Sample or Track) |
| **Routing** | Each output's function and the channel(s) it reads |
| **Outputs** | Settings for the functions that need them: step size for CNT / BIN, mode and times for ENV |
| **CV Mod** | Function-by-CV per output, and preset selection by CV with a load trigger |
| **Audio** | The ORN8's audio applets, as in Quadrants; while on this page all buttons and encoders control audio, and A or B at its top level moves to the previous or next page. Long-press A or Y for presets, or X for Routing. The audio setup is shared with Quadrants |
| **Scope** | Eight on/off traces of channels and outputs, in Roll, Sweep or Trig mode; also the screensaver |

Long-press **A** (or press **Y**) for the preset overlay: 100 banks of 8 slots, saved to the SD card or internal flash. Saving a slot also stores the audio applet setup; loading applies it unless **Audio** is set to Off on the CV Mod page. Press **X** to jump to the Routing page and back.

### Output functions

| Function | Reads | Output |
| --- | --- | --- |
| OUT / NOT | one channel | its gate, or the inverse |
| AND, NAND, OR, NOR, XOR, XNOR | two channels | the logic combination |
| TOGL | one channel | flips on each rising edge (divides by two) |
| LTCH | two channels | set by the first, reset by the second; simultaneous edges are ignored |
| CNT | all four | stepped CV: how many channels are high (0–4) |
| BIN | all four | stepped CV: CH1–CH4 as a 4-bit number (0–15) |
| ENV | one channel | 0–5 V envelope, AD or ASR |

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

Clock T1 and set the four channels to divide it by powers of two, each with a gate half its period long:

| Channel | Source | Divider | Len by | Length |
| --- | --- | --- | --- | --- |
| CH1 | T1 | /2 | Clock | 50% |
| CH2 | T1 | /4 | Clock | 50% |
| CH3 | T1 | /8 | Clock | 50% |
| CH4 | T1 | /16 | Clock | 50% |

Set an output to **BIN** with step **1/3V**. The channels now count like the digits of a binary counter, so the output steps down 15, 14, 13 … 0, one step per clock, and repeats every 16 clocks: a descending staircase from 5 V to 0 V. On the Scope page, C1–C4 show the counter pattern, each half the speed of the one before.

Use it as a stepped sweep for a filter cutoff, or with **Semi** steps into a VCO for a descending chromatic run. Clock-relative length needs two clocks to measure the tempo, so the very first step may be off.

#### A generative melody

Odd dividers make the channels drift in and out of phase with each other:

| Channel | Source | Divider | Length |
| --- | --- | --- | --- |
| CH1 | T1 | x1 | 100 ms |
| CH2 | T1 | /3 | 300 ms |
| CH3 | T1 | /4 | 400 ms |
| CH4 | T1 | /5 | 600 ms |

Set an output to **BIN** with **Semi** steps, and patch it through a quantiser set to a scale into a VCO's 1 V/oct input. Use output E (XOR of CH1 and CH2) as the trigger for your envelope, or set another output to **ENV** in AD mode on CH1 for a percussive VCA envelope.

Each clock produces a note from the current combination of high channels, and the pattern repeats only every 60 clocks. Adding **Prob** on CH1, or CV on a divider, makes it evolve.
