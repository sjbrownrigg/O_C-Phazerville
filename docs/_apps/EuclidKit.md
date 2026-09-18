---
layout: default
---
# EuclidKit

*EuclidKit* is a 4-track drum sequencer with two switchable engines, one per-track output on A to D, clocked from TR1 with TR2 as reset:

- **Euclidean** -- each track generates its own looping rhythm with the Bjorklund (Euclidean) algorithm, at its own step length (polymetric), with per-step manual overrides.
- **Grids** -- a 4-channel take on Mutable Instruments Grids: a single shared X/Y map position plus a Chaos amount drives all four tracks, each reading one of the three Grids drum parts (BD/SD/HH) at its own density.

The **engine** (Euclidean or Grids) is a global setting shared by all four tracks. A `EUC` / `GRD` badge in the top-right of the display always shows the current engine, and you switch it on the **Engine page** -- the last item in the settings list (see Navigation).

### Navigation

Per-track settings form one continuous list under the Left Encoder: turn it (when not editing) to move through every field in order, rolling from one page to the next. The gauge icon (top-right) inverts when you are off the main page. The Up / Down buttons change which track is focused (A→B→C→D).

The list order, per engine:

- **Euclidean**: Length / Beats / Rotation / Mute  →  Probability / CV target  →  CV calibration  →  Engine
- **Grids**: Fill / Part / Mute  →  Probability / CV target  →  X / Y / Chaos  →  CV calibration  →  Engine

### Global gestures (both engines)

| Gesture                    | Action                                                              |
| -------------------------- | ------------------------------------------------------------------- |
| Left Encoder — turn        | Navigate the settings list; while editing, adjust the selected value |
| Left Encoder — press       | Toggle edit mode for the selected parameter                         |
| Left Encoder — long press  | Toggle mute on the focused track                                    |
| Up Button — press          | Focus the previous track (A ← B ← C ← D ← A)                         |
| Down Button — press        | Focus the next track (A → B → C → D → A)                             |
| Up Button — long press     | Open / close the Preset screen (see Presets)                        |

To switch engine, scroll the Left Encoder to the **Engine** page (end of the list), press to edit, and turn. The main App Menu / global save is the usual firmware gesture (hold Up + press the Right Encoder), not specific to this app.

### Euclidean-mode gestures

| Gesture                    | Action                                                              |
| -------------------------- | ------------------------------------------------------------------- |
| Right Encoder — turn       | Move the step cursor along the focused track's pattern              |
| Right Encoder — press      | Toggle the step at the cursor on/off (a manual override)            |
| Down Button — long press   | Clear all manual step overrides on the focused track               |

### Grids-mode gestures

Grids patterns come from the shared map rather than per-step editing, so the Right Encoder (step cursor / step toggle) and the Down-button override-clear are inactive. The Grids performance controls -- **X / Y / Chaos** -- live on their own page in the settings list (scroll the Left Encoder past the modulation page); see Navigation.

### Firmware global gestures (reminder)

These are built into the Phazerville firmware and behave the same in every app -- they are **not** specific to EuclidKit, but are easy to forget. On O_C hardware the modifier button is **Up** (labelled **A** on T4.1). See the full [UI Gestures](Hemisphere-Gestures) page for the complete list.

| Gesture                                            | Action                                                                                 |
| -------------------------------------------------- | -------------------------------------------------------------------------------------- |
| Hold **Up** + press the **Right Encoder**          | Open the main **App Menu** -- this is how you leave EuclidKit and switch to another app |
| Hold **Up** + press the **Left Encoder**           | Open **I/O Settings** for the current app (input mapping, output ranges, etc.)          |
| **Long-press the Right Encoder** while in the App Menu | **Save** everything to storage -- the selected app plus its settings and presets, scales, patterns, etc. ([Saving State](Saving-State)) |

Saving is how your live kit and all four preset slots survive a power-cycle: open the App Menu and long-press the Right Encoder.

### I/O

|     | 1                                          | 2                                            | 3                    | 4                    |
| --- | -------------------------------------------- | ---------------------------------------------- | -------------------- | -------------------- |
| TR  | Master clock -- advances the sequence      | Reset -- returns to step 0                     | (unused)             | (unused)             |
| CV  | Modulates Track A (target set per track)   | Modulates Track B                              | Modulates Track C    | Modulates Track D    |
| OUT | Track A gate/trigger                       | Track B gate/trigger                            | Track C gate/trigger | Track D gate/trigger |

Each CV input maps one-to-one to its track (CV1 → Track A, and so on). What it modulates is set per track by that track's `CV target`. Modulation is **absolute**: the calibrated input span (see [CV Calibration](#cv-calibration)) is mapped straight onto the target parameter's full range, so the input voltage *sets* the value (0V → minimum, full-scale → maximum) and the knob for that parameter is overridden while CV drives it. The on-screen value for a modulated parameter updates live, so you get direct visual confirmation as the voltage moves. In Grids mode, X / Y / Chaos are global, so pointing a track's CV target at X or Y lets that CV input sweep the shared map position -- the classic Grids live-morph.

> **Input range note:** the O_C CV inputs read roughly 0V up to about +6V of positive voltage; higher voltages saturate. If your CV source swings wider (e.g. a 0-10V source), use the calibration below to capture its usable span so the full parameter range stays reachable.

### The Euclidean engine

On each TR1 pulse every track's step counter advances and wraps at that track's own **Length**, so tracks of different lengths (e.g. 13 vs 16) drift in and out of phase over time rather than resetting together. TR2 resets all counters at once.

Each track's pattern comes from three settings via the Euclidean (Bjorklund) algorithm: **Length** (steps in the loop), **Beats** (active hits, spread as evenly as possible), and **Rotation** (rotates the pattern). Changing any of them regenerates the pattern immediately. Individual steps can be hand-toggled with a Right Encoder press; a toggled step is "locked" and survives further Length/Beats/Rotation edits until you long-press Down to clear the overrides.

### The Grids engine

All four tracks share one 32-step pattern clock and a single position on the Grids "map":

- **X** / **Y** (global) -- position on the 5x5 map of drum patterns; moving them morphs the feel of the whole kit.
- **Chaos** (global) -- adds per-loop randomness to the generated densities.
- **Fill** (per track) -- density/threshold; higher fill = more hits get through.
- **Part** (per track) -- which of the three Grids parts the track reads: `BD`, `SD`, or `HH`. Defaults are BD/SD/HH/HH, so track D doubles the hi-hat -- give it a different Fill (or CV) for variation.

Probability and the per-track CV targets (Density / X / Y / Chaos / Prob) layer on top of the Grids output.

### CV Calibration

Because CV control is absolute, the module needs to know the voltage span of your CV source so it can map that span onto each parameter's range. This is a **learn / capture** step on the **CV calibration** page (scroll the Left Encoder to it -- second-to-last in the list, just before Engine). It is a global, per-module setting (shared by all four inputs) and is **not** part of preset snapshots.

The calibration page shows the **live input voltage** (`In`) for the currently focused track's CV input, plus the selected endpoint (`Lo` or `Hi`). To calibrate:

1. Patch your CV source (e.g. an attenuator) into the focused track's CV input.
2. Turn the Left Encoder to select **Lo**, set your source to its minimum, and **press the Left Encoder** to capture it.
3. Turn to select **Hi**, set your source to its maximum, and **press** to capture that.

From then on, that captured span maps to `0V-end → parameter minimum`, `max-end → parameter maximum`, for every CV target. Watch the live `In` reading while sweeping your source to confirm the range (and to see where the input saturates). The default span is roughly 0V to 6V. Remember a [global save](#firmware-global-gestures-reminder) to keep the calibration across power cycles.

### Settings

Common to both modes:

| Setting       | Meaning                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------ |
| `Mute`        | Silences the track's output without stopping the sequence                                 |
| `Probability` | Chance (0-100) that a scheduled hit fires. 100 = always                                    |
| `CV target`   | What the track's CV input modulates (engine-dependent list, see above)                    |
| `Engine`      | Global engine select: `Euclid` or `Grids` (last item in the settings list)                |

Euclidean-only: `Length`, `Beats`, `Rotation` (per track).
Grids-only: `Fill`, `Part` (per track); `X`, `Y`, `Chaos` (global).

### Presets

EuclidKit has **4 preset slots**, each a complete snapshot of everything -- all four tracks *and* the global settings, including the mode. So one slot can hold a Euclidean kit and another a Grids kit, and recalling switches engine and all.

Long-press the **Up button** to open the Preset screen. On the Preset screen:

| Control                    | Action                                                        |
| -------------------------- | ------------------------------------------------------------- |
| Left Encoder (turn)        | Select a slot (1-4)                                           |
| Up Button                  | **Save** the live state into the selected slot (shows "SAVED")|
| Down Button                | **Load** the selected slot into the live state                |
| Left Encoder (press)       | Exit the Preset screen (Up long-press also exits)            |

Load is on the Down button so a reflexive press does the non-destructive action. The slot last loaded or saved is marked with a small dot.

> **Remember to do a global save too.** Saving to a preset slot only stores it in working memory -- it does **not** survive a power cycle on its own. To make your slots (and the live state) persist, you must also do the firmware's global save: open the App Menu (hold **Up** + press the **Right Encoder**) and **long-press the Right Encoder**. Without that global save, all four slots are lost on power-off. (See [Firmware global gestures](#firmware-global-gestures-reminder) above and [Saving State](Saving-State).)

### Screensaver display
Uses the standard input/output meter screensaver, showing activity on all four channels.
