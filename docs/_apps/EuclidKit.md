---
layout: default
---
# EuclidKit

*EuclidKit* is a 4-track polymetric Euclidean drum sequencer, giving each of the four outputs (A to D) its own independently-looping rhythm track generated with the Bjorklund (Euclidean) algorithm, with the ability to hand-edit individual steps on top of the generated pattern.

Each track can have its own step length, independent of the others, so tracks can run at different loop lengths against a single shared clock -- a simple way to get slowly-evolving polymetric grooves out of a small number of parameters.

### Controls

|       | Left Encoder                                                                                                          | Right Encoder                                                                              |
| ----- | ------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------- |
| TURN  | Navigate between Length / Beats / Rotation / Mute (when not editing). While editing, adjusts the selected parameter's value. | Move the step cursor along the focused track's pattern.                                    |
| PRESS | Toggle edit mode for the selected parameter.                                                                            | Toggle the step at the step cursor on/off (a manual override -- see below).                |
| LONG  | Toggle mute on the focused track.                                                                                       | App selection menu                                                                          |

|            | Up Button                                                | Down Button                                                                                                            |
| ---------- | ----------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| PRESS      | Move focus to the previous track (A ← B ← C ← D ← A)         | Move focus to the next track (A → B → C → D → A)                                                                        |
| LONG PRESS |                                                               | Clear manual step overrides on the focused track, snapping it back to the pure Euclidean pattern from Length/Beats/Rotation |

### I/O

|     | 1                                          | 2                                            | 3         | 4         |
| --- | -------------------------------------------- | ----------------------------------------------- | ----------- | ----------- |
| TR  | Master clock -- advances all four tracks   | Reset -- returns all tracks to step 0          | (unused)  | (unused)  |
| CV  | (unused)                                   | (unused)                                       | (unused)  | (unused)  |
| OUT | Track A gate/trigger                       | Track B gate/trigger                            | Track C gate/trigger | Track D gate/trigger |

CV1 to CV4 are intentionally left unused in this version, reserved for future per-track probability/accent modulation.

### Description

On each pulse into TR1, every track's internal step counter advances by one and wraps at that track's own Length -- so, for example, a 13-step track and a 16-step track drift in and out of phase with each other over time, rather than resetting together every bar. TR2 resets all four counters to step 0 at once.

Each track's pattern is generated from three settings using the Euclidean (Bjorklund) algorithm:

- `Length` -- number of steps in the track's loop
- `Beats` -- how many of those steps are active hits, spread as evenly as possible across the loop
- `Rotation` -- rotates the generated pattern

Changing any of these three regenerates that track's pattern immediately. Individual steps can also be hand-toggled with a Right Encoder press; a manually-toggled step is "locked" and survives further Length/Beats/Rotation edits, rather than being overwritten by the next regeneration. Long-pressing Down clears all manual overrides on the focused track and returns it to the pure Euclidean shape.

### Settings (per-track)

| Setting    | Meaning                                                                 |
| ---------- | ------------------------------------------------------------------------ |
| `Length`   | Number of steps in the focused track's loop (1 to 16)                   |
| `Beats`    | Number of active (hit) steps in the Euclidean pattern                   |
| `Rotation` | Rotation offset applied to the generated pattern                        |
| `Mute`     | Silences the focused track's output without stopping its step counter   |

### Screensaver display
Uses the standard input/output meter screensaver, showing activity on all four channels.
