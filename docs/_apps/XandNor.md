---
layout: default
---
# (X)AND(N)OR

> **Hardware: Teensy 4.1 [O.R.N.8](https://www.tlm-audio.com/product-page/oc-4-1-phazerville) required.** To get the best out of it, (X)AND(N)OR is built around the O.R.N.8's more generous I/O: eight outputs (A–H) for its gates, logic and CV functions, and eight CV inputs (1–8), so every channel setting can have its own modulation. It also uses the O.R.N.8's X and Y buttons, its audio applets and its microSD slot for preset banks. It is included only in Teensy 4.1 (T41) builds; 4-output Teensy 4.0 and 3.2 modules have neither the I/O nor the save space for it.

*(X)AND(N)OR* is a rhythm generator built from gate logic: from a single clock, or up to four triggers, it makes up to eight related rhythms, plus stepped voltages and modulation that move with them.

In Eurorack terms it is mainly a gate processor and logic module, but in use it works as a rhythm generator. Give it one clock, or a few triggers, and it returns a whole set of related rhythms: nested gate lengths, off-beats, divisions, the slices between them, and logic combinations of all of these. On top of that it makes stepped CV (CNT and BIN) and modulation (ENV and the Mods), all locked to the same rhythm. It isn't a function generator: its envelopes and LFOs are there to support the rhythms. In short, it turns a simple clock into a full set of interlocking rhythms and matching CV, without a sequencer.

It works by turning triggers into gates and combining them. Four gate channels (CH1–CH4) each take a trigger from any input and produce a gate from 1 ms to 10 s long, shaped by a divider, delay, probability and ratchets. Eight outputs (A–H) each carry a channel's gate, a logic combination of two channels, a toggle or latch, a stepped CV built from all four channels, an envelope, a Mod's voltage, or a sampled CV. Almost every setting can be modulated by CV.

The app also hosts the O.R.N.8's audio applets, has a built-in logic-analyser scope that doubles as its screensaver, and stores 100 banks of 8 presets on the SD card.

Panel labels used here: outputs **A–H**, CV inputs **1–8** (shown on screen as CV 1–CV 8), trigger inputs **TR 1–TR 4**. The Up and Down buttons are labelled **A** and **B** on the O.R.N.8.

## Contents

- [Quick start](#quick-start)
- [Controls](#controls)
- [Sources](#sources)
  - [Trigger sources](#trigger-sources)
  - [CV sources](#cv-sources)
- [Overview](#overview)
- [Channel pages (CH1–CH4)](#channel-pages-ch1ch4)
  - [CV scaling](#cv-scaling)
- [Routing](#routing)
  - [Default patch](#default-patch)
  - [Reading the logic](#reading-the-logic)
- [Outputs](#outputs)
  - [Function by CV](#function-by-cv)
  - [Function CV examples](#function-cv-examples)
- [Mods](#mods)
  - [Mod examples](#mod-examples)
- [Global](#global)
- [Audio](#audio)
- [Scope](#scope)
- [Presets and banks](#presets-and-banks)
- [Patch examples](#patch-examples)
  - [Using BIN](#using-bin)
  - [Ratchet rolls on demand](#ratchet-rolls-on-demand)
  - [Off-beats and swing from one clock](#off-beats-and-swing-from-one-clock)
  - [Driving the audio applets](#driving-the-audio-applets)

## Quick start

The default patch makes music from one clock, so you can hear the app before changing anything.

1. **Clock it.** Patch a steady clock (quarter notes, around 120 BPM) into **TR 1**. On the Overview, the CH1–CH4 boxes flash with every clock.
2. **Listen.** Patch **A** to a kick or any trigger input: it fires a 10 ms trigger on every clock. Patch **E** to a hi-hat: it fires a short gate just after each clock (the slice between CH1 and CH2). **H** is high in the gap after CH4's 250 ms gate, so it makes a good off-beat gate. The [Default patch](#default-patch) lists all eight.
3. **Change the logic.** Press **X** for Routing, move to E's function with the left encoder, and turn the right encoder: AND, OR, XNOR and the others each give a different pattern at once.
4. **Double up.** Press **B** twice from the Overview to reach CH2. Turn the left encoder one step to the divider (next to Source), then turn the right encoder to **x2**. CH2 now fires twice per clock, so B and E play eighth notes.
5. **Loosen it.** On CH3, set **Prob** to 50%: C now plays only some of the beats.
6. **Watch it.** Go to the Scope, the last page (from the Overview, press **A** once: the pages wrap round): every channel and output is drawn in time, with no patching.
7. **Keep it.** Press **Y** (or long-press **A**), choose a slot with the right encoder and press **A** to save. Loading an empty slot brings back the default patch.

## Controls

| Control | Action |
| --- | --- |
| **A** / **B** | Previous / next page |
| Left encoder turn | Move the cursor between fields (pages with long lists scroll to follow). Past the last field it wraps round to the first, and back |
| Right encoder turn | Change the selected value directly; there is no press to start editing |
| Long-press **A**, or **Y** | Open or close the preset overlay |
| **X** | Jump to the Routing page; press again to go back |
| **Z** | Not used by the app: it remains the platform's menu modifier (Z + encoder press) |

The field cursor is a steady underline. On the Audio page the audio engine takes all short presses; see [Audio](#audio).

Pages, in order: **Overview**, **CH1–CH4**, **Routing**, **Outputs**, **Mods**, **Global**, **Audio**, **Scope**. The list wraps, so **A** from Overview goes straight to Scope.

- **Tip:** **X** is the quickest way to try ideas: jump to Routing, change a function, then press **X** again to get back to the page you were on.

## Sources

Every setting that takes a trigger or a CV picks it from a list. The lists are the platform's, with (X)AND(N)OR's own signals added straight after **CV 8**, so they are easy to find.

### Trigger sources

Used by a channel's **Source** and **Reset**, a Mod's **Trig** or **Sync**, **Rst all**, the preset **Load on** trigger and the Scope trigger.

| Source | What it is |
| --- | --- |
| **TR 1**–**TR 4** | The trigger inputs |
| **CV 1**–**CV 8** | The CV inputs used as gates: high above 1.25 V |
| **CH1>**–**CH4<** | Another channel: **>** fires when its gate starts, **<** when it ends. Only in a channel's Source and Reset, a Mod's Trig or Sync, and Rst all |
| **OutA**–**OutH** | (X)AND(N)OR's own outputs, read back one tick later |
| **VrtI**–**VrtP** | The platform's virtual outputs, shared with Quadrants |
| **M 1**… | The platform's MIDI mappings |
| **CL1**, **CL4**, **RUN**, **RST** | The platform's clock, which follows Quadrants' clock setup; it doesn't run in this app |
| **–** | Off |

- **Tip:** a channel source (**CH1>** and so on) is read one tick (60 µs) after that channel's edge. That never matters for sound, but it does for resets: see the **Reset** row on the [channel pages](#channel-pages-ch1ch4).

### CV sources

Used by every CV row in the app: the channel pages, the Outputs page (function CV, ENV times, S&H), the Mods page (Period and Level CV, a Slew's Input) and the Global page's Preset CV.

| Source | What it is |
| --- | --- |
| **CV 1**–**CV 8** | The CV input jacks |
| **Mod1**–**Mod4** | The app's own modulators: LFOs, random voltages, slews and envelopes, set up on the [Mods](#mods) page |
| **OutA**–**OutH** | (X)AND(N)OR's own outputs, read back one tick later, so one output can modulate a channel or another output with no patch cable. Gate functions give 0 or 5 V, **ENV** 0–5 V, **CNT** / **BIN** their stepped voltage |
| **VrtI**–**VrtP** | The platform's virtual outputs, shared with Quadrants. Quadrants' clock writes some of them but doesn't run here; some audio applets write CV to them (TuneTracker's detected pitch, for example), which does |
| **M 1**… | The platform's MIDI CV mappings, set up in its MIDI settings |
| **G*** | Noise: a new random value on every tick |
| **–** | Off |

- **Tip:** for modulation with no cables at all, use a **Mod**. An **ENV** output as a CV source (for example H as CH1's Length CV) is the next quickest.
- The audio applets have their own source list, which includes the outputs (as **#A**–**#H**) but not the Mods or the channel sources.

## Overview

Boxes for CH1–CH4 and outputs A–H fill while they are high, and the last preset saved or loaded is shown underneath. A small box in the header of each channel page shows that channel's gate in the same way.

- **Tip:** if nothing seems to happen, look here first. No flashing CH boxes means no trigger is reaching the channels; flashing boxes but silent outputs points at the Routing.

## Channel pages (CH1–CH4)

Each channel turns triggers into gates. The page is a scrolling list:

| Row | Setting |
| --- | --- |
| **Source** | The trigger: any of the [trigger sources](#trigger-sources), including another channel's start (**CH1>**) or end (**CH1<**). Next to it, the **divider**: turning right goes x1, x2, x3 … (multiply), turning left /2, /3 … (divide) |
| **Reset** | A trigger that restarts the channel's counting: the next trigger is the first of its division (so /4 lines up with the bar) and the first step of any Euclidean pattern, and triggers waiting in the delay queue are dropped. A gate already running finishes. The same sources as **Source**, including CH1>…CH4<; default off. A channel used as the reset arrives one tick (60 µs) after that channel's edge, so a clock at the same moment has already been counted and the division restarts on the following clock. For a reset that lines up with the clock, use an input |
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

**Tips**

- **Tempo-proof gates:** set **Len by** to **Clock** at 50% for square waves that follow the clock when it speeds up or slows down. They are ideal for feeding other modules' clock inputs.
- **Ratchets need room:** ratchets split the gate, so the gate's length sets their speed. Eight ratchets in a 50 ms gate are a buzz; in a 400 ms gate they are a roll. Each pulse needs about 2 ms, so a 10 ms gate fits at most five.
- **Human feel:** a **Prob** of 70–85% on a hi-hat channel drops the odd hit without sounding random. Lower values suit fills and ghost notes.
- **Instant pulse-width modulation:** patch an LFO into CV 1, set the Source to **CV 1** and Length to **Level**, then turn **Thresh**: the gate gets wider or narrower.
- **A free-running clock:** set CH1's Source to its own end (**CH1<**), Length 10 ms and **Delay** 240 ms: a 10 ms pulse every 250 ms with nothing patched, and the Delay CV becomes a tempo control. (With no delay, the gate restarts as soon as it ends, so it is high almost all the time.)
- **Sample or Track:** use **Sample** when each gate should keep one value, such as random lengths. Use **Track** for long gates that should change while they play, such as a ratchet roll that speeds up.

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

- **Tip:** CV adds to the setting on the page, so set the page value to where the modulation should start. For example, set **Prob** to 0% and let the CV bring the channel in.

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
| MOD | none | a Mod's voltage (Mod1–Mod4, chosen on Outputs) |
| S&H | one channel | a CV sampled each time the channel starts |

### Default patch

A fresh install and empty preset slots start here. All four channels take TR 1, with lengths 10, 50, 100 and 250 ms, so one clock fans out into nested gates and the slices between them:

| Output | Function | Result on each clock |
| --- | --- | --- |
| A–D | OUT 1–4 | 10, 50, 100 and 250 ms gates |
| E | XOR 1 2 | 10–50 ms |
| F | XOR 2 3 | 50–100 ms |
| G | XOR 3 4 | 100–250 ms |
| H | NOT 4 | from 250 ms until the next clock |

### Reading the logic

A quick guide to what each function does with rhythms:

- **XOR** of two nested gates (one inside the other) gives the slice between them: the default patch's E–G. Change one channel's Length to move the slice.
- **AND** fires only where two rhythms coincide. With CH1 at /3 and CH2 at /4 (short gates), **AND 1 2** hits once every 12 clocks: an accent for a three-against-four pattern.
- **OR** merges two rhythms into one, but overlapping gates merge into one long gate. To keep every hit separate, keep both lengths short (10 ms is a good start).
- **NOT** fills the gaps: a long gate's inverse is an off-beat, as on H.
- **NAND**, **NOR** and **XNOR** are the inverses of AND, OR and XOR. Use them when you want the gaps rather than the hits.
- **TOGL** divides by two and alternates: high for one trigger, low for the next. It is useful for left/right or A/B switching.
- **LTCH** turns two rhythms into one gate: it opens on the first channel and closes on the second. For example, a gate from the bar line (CH1 at /4) to a later hit (CH2 with a delay).

## Outputs

One row per output: its letter, the function in use, then that function's settings. Routing sets which function and channels each output uses; this page sets everything else. When a function CV is moving the function, the row shows the one in use right now.

| Function | Settings on its row |
| --- | --- |
| OUT … XNOR (logic) | The **function CV** and its depth: see [Function by CV](#function-by-cv) |
| TOGL, LTCH | None: the row is blank and the cursor skips it |
| CNT, BIN | Step size: **Semi** (a semitone, for pitch), **1/3V** or **1V** |
| ENV | Mode **AD** (each rising edge runs attack then decay, whatever the gate length) or **ASR** (attack, hold while the gate is high, then release), attack (default 2 ms) and decay or release (default 200 ms) on the 1 ms – 10 s scale. Two more rows below it, **Att** and **Dec**: the CV for the attack and decay times, each a source and depth in the same columns as the other outputs' CV |
| MOD | Which Mod, **Mod1**–**Mod4**: the output carries that Mod's voltage |
| S&H | A CV source and depth, sampled each time the output's channel starts and held until the next |

```
A NAND  CV 3       100%
C TOGL
D BIN   Semi
E ENV  AD 2.0ms 200ms
  Att   Mod1       100%
  Dec    -         100%
F MOD   Mod1
G S&H   CV 2       100%
```

- The cursor remembers its output and field. If a function change on Routing removes that field, the cursor moves to the nearest field before it.
- **MOD** and **S&H** are CV outputs. On the Overview, the Routing page and the Scope they show as high while above 1.25 V.
- **Att** and **Dec** CV move the time 8 steps per volt at 100% depth, as Length CV does: +5 V covers the whole 1 ms – 10 s range.

**Tips**

- **A stepped accent:** in the default patch, set H to **CNT** with **1/3V** steps. Each clock it steps down 1.33, 1, 0.67, 0.33 V as the channels end one by one: a stepped decay for a VCA or an accent input.
- **AD or ASR:** **AD** ignores the gate length, so it suits short triggers and percussion. **ASR** follows the gate, so the channel's Length becomes the note length, and Length CV then shapes the envelope as well.
- **An LFO on a jack:** set an output to **MOD** with **Mod1**: the default 1 s sine LFO is on that output at once, ready for a filter or VCA. It also lets the audio applets read a Mod (see [Driving the audio applets](#driving-the-audio-applets)).
- **Rhythmic random notes:** set G to **S&H** on CH1 with its source **Mod2** (Rand), and set Mod2's Period to 10 ms so it changes all the time. Each CH1 gate catches a new value: random notes in time with CH1, ready for a quantiser.
- **Envelopes that open up:** give an ENV output's **Dec** row a slow LFO (**Mod1**): the decay gets longer and shorter, so hits go from tight to open and back.

### Function by CV

Each logic output's function CV moves its function on from the one set on the Routing page, through the eight logic functions in this order:

**OUT → NOT → AND → NAND → OR → NOR → XOR → XNOR**

- At 100% depth each step takes 0.625 V, so +5 V covers all eight. Negative voltage, or negative depth, moves back towards OUT. The result stops at either end rather than wrapping.
- The function changes on the next tick (60 µs), so audio-rate modulation is possible.
- The output keeps the channels set on the Routing page. A one-channel function (OUT, NOT) reads the first; a two-channel function reads both, so set both channels on Routing if the CV can reach the two-input functions.
- Only logic functions have a function CV. **TOGL**, **LTCH**, **CNT**, **BIN**, **ENV**, **MOD** and **S&H** keep state or output CV, so switching them by CV wouldn't behave predictably.
- Several outputs can share one CV source, each with its own depth.
- The sources are the usual [CV sources](#cv-sources).

**Depth for gate sources.** A gate output is 0 or 5 V, so at 100% it jumps all the way to XNOR. To step one function per gate, set the depth to about **13%**; two functions need about **25%**. The depth readout shows the percentage actually applied.

### Function CV examples

- **Flip a logic output with a gate:** set E to **AND 1 2** on Routing, then give E the source **OutH** at about 13%. While H is high, E becomes **NAND**, its exact inverse.
- **A function sequencer:** give output F a slow stepped random voltage, or a **BIN** output at **1/3V** steps from this app, at 100% depth. F then walks through the logic functions as the voltage changes, while the Overview and Scope show the result.
- **No cables:** set F to **AND 2 3** on Routing, then give it the source **Mod2** (Rand) at about 40% depth. Every 500 ms a new random value picks a function a few steps either side of AND, so the pattern changes by itself.
- **Feedback:** an output modulating its own function (A with source **OutA**) can lock up or oscillate depending on its channels. It's unpredictable, which can be the point.

## Mods

Four internal modulators, **Mod1**–**Mod4**, that any CV row can use: pick them in a CV source list, straight after CV 8. To send one to a jack, set an output to **MOD**. They are part of the patch, so presets save and load them. Each has a type; each type shows its own settings:

| Type | What it does | Settings |
| --- | --- | --- |
| **LFO** | A free-running oscillator | **Wave**: Sine, Tri, Saw (rising), Ramp (falling), Sqr. **Period**: 1 ms to 60 s, with its **CV**. **Level** and polarity, with its **CV**. **Sync**: a trigger that restarts the cycle |
| **Rand** | A random voltage, held until the next change (sample and hold) | **Trig**: a new value on each trigger; with no trigger, a new value every **Period** (with its **CV**). **Level** and polarity, with its **CV**. **Glide**: Off, or the time a full-scale change takes, for smooth random |
| **Slew** | Another signal, smoothed | **Input**: any CV source (a CV input, an output, another Mod) with its depth. **Rise** and **Fall**: Off, or the time to move 5 V |
| **Env** | An attack/decay envelope | **Trig**: what starts it; a new trigger restarts the attack from the current level. **Attack**, **Decay**: Off (instant) or 1 ms to 10 s. **Level**, with its **CV** |

- **Level** is 0–100% of 5 V. Polarity: **Bi** swings ±Level around 0 V; **Uni+** goes from 0 V up to +Level; **Uni-** from 0 V down to −Level.
- The **CV** row under Period or Level is a CV source and depth, like the channel pages' CV rows. **Period CV** moves the period 8 steps per volt at 100% depth (+CV lengthens it, as Length CV does); while it is set, an arrow shows the period in use. **Level CV** adds 100% per +5 V at 100% depth, as Prob CV does.
- Each Mod's first row shows its type, with a meter of its output beside it: a bar to the right of the centre line for positive voltages, to the left for negative.
- **Trig** and **Sync** offer the same sources as a channel's Source, including the channels (**CH1>** a channel's start, **CH1<** its end).
- The defaults are one of each: **Mod1** a 1 s sine LFO, **Mod2** a random voltage every 500 ms, **Mod3** a slew (no input yet, 100 ms each way), **Mod4** an envelope on TR 1 (2 ms attack, 200 ms decay). Presets saved before the Mods existed load with these.
- The CV row's own depth still applies, so one Mod can drive several targets by different amounts, or inverted.
- To see a Mod, set a Scope trace to **M1**–**M4**.

**Tips**

- **Hear a Mod at once:** the defaults are ready to use. Set any CV row's source to **Mod1** for a 1 s sine wave, or to **Mod2** for a new random value every 500 ms.
- **Which polarity:** use **Bi** for wobble around the setting on the page, and **Uni+** or **Uni-** when the modulation should only add or only take away (a delay that should never go below the setting, for example).
- **Smooth random:** a **Rand** with **Glide** about the same as its Period wanders smoothly instead of jumping.
- **Mods modulating Mods:** give Mod1's **Period CV** the source **Mod2** (Rand) for an LFO that keeps changing speed, or set Mod1's **Level** to 0% and give its **Level CV** the source **Mod4** (Env) with a long decay, for an LFO that comes in on each TR 1 trigger and fades away.
- **Faster with negative CV:** Period CV lengthens the period as the voltage rises. For "more voltage = faster", set its depth negative.

### Mod examples

- **Swing that breathes:** give CH2's Delay CV **Mod1** (a slow LFO, set to **Uni+**) at a low depth, so CH2 drifts late and back.
- **Random gate lengths:** set CH1's Length CV to **Mod2**, with Mod2's **Trig** on the same input as CH1's Source (TR 1): the Mods update before the channels, so each gate starts with a fresh random length.
- **Fading density:** set CH1's Prob to 0% and its Prob CV to **Mod4** (Env) triggered by **CH3>**, with a long decay: right after each CH3 gate CH1 fires every time, then thins out as the envelope falls.
- **Slewed function sweep:** a **Slew** with Input **OutG** (a BIN output) smooths the staircase into a ramp, which can then drive another channel's Length.
- **Bar-locked LFO:** an **LFO** with Sync on the clock divided by a channel (**CH4>** with CH4 at /16) restarts on every bar.


## Global

Settings that stay put when a preset loads, so whatever drives preset changes and resets keeps working across them. They are stored with the working patch.

| Row | Setting |
| --- | --- |
| **Rst all** | The global reset: a trigger that resets every channel (as each channel's own **Reset** does), restarts every LFO and every Rand's own clock, and clears TOGL and LTCH outputs |
| **Preset** | A CV source and depth that pick slot 1–8 of the current bank (0–5 V, about 0.6 V per slot) |
| **Load on** | The trigger that loads the slot the Preset CV points to. With no Preset CV set, each trigger steps to the next saved slot ("next") |
| **Audio** | Whether loading a preset also loads its audio setup (default On) |

**Tips**

- **Line everything up:** send **Rst all** the same pulse as your sequencer's reset to bring dividers, Euclidean patterns and LFOs back in line together. A channel used as **Rst all** arrives one tick late, as with a channel's own Reset, so use an input for a reset that lines up with the clock.
- **Play a song from presets:** save each section in its own slot, leave **Preset** off and set **Load on** to a trigger that fires once per section (a channel at /16, for example). Each trigger moves on to the next saved slot.
- **Keep the sound:** set **Audio** to Off to change rhythms with a preset load while the sound stays the same.

## Audio

The O.R.N.8's audio applets, exactly as in Quadrants' audio setup. While (X)AND(N)OR is running, the audio applets follow CV and triggers here too, whichever page you are on.

On this page all short presses of A, B, X and Y and both encoders go to the audio engine. Pressing **A** or **B** at its top level moves to the previous or next page. Long-press **A** or **Y** for presets, or **X** for Routing.

The audio setup is shared with Quadrants: changes made in either app appear in the other.

The audio applets can take their triggers and CVs from (X)AND(N)OR's outputs with no patching: see [Driving the audio applets](#driving-the-audio-applets).

- **Tip:** the applets already play at full level and there is no master volume. If a drum sounds too quiet, add **Dynamics** in the next slot and raise its makeup gain; a **VCA** above 0 dB also adds gain, with clipping once past full scale.

## Scope

A logic analyser of the app's own signals, with no patching. The status line shows mode, trigger, timebase, capture and HOLD; below it are up to eight traces.

| Setting | Options |
| --- | --- |
| Mode | **Swp** (default): the picture stays still and is redrawn in place as new data arrives. **Roll**: scrolls, newest on the right. **Trig**: each sweep starts at the trigger and is drawn as it is captured |
| Trigger | Any input or output (default TR 1), or none for free-running |
| Timebase | **Auto**, or 5 ms to 10 s per screen. Auto fits two trigger periods in Swp and Trig, and is a fixed 2 s in Roll so the scroll speed never depends on the clock |
| Capture | **Pk** (default): each column records everything that happened during it, so no pulse is missed. **Sm**: each column takes a single sample, as the Scope applet does |
| Traces | Each of the eight shows **C1–C4** (channels), **A–H** (outputs), **V1–V8** (CV inputs) or **M1–M4** (the mod sources), or "--". CV and mod traces are drawn from −5 V to +5 V. Default: C1–C4 and E–H. In Pk mode a CV trace shows each column's full range, so fast modulation appears as a band |

- **Left encoder press:** zoom, 8 → 4 → 2 → 1 traces, each taller as fewer share the screen. Moving the cursor through the traces scrolls the view.
- **Right encoder press:** hold (freeze) the display.
- Changing mode, trigger, timebase or capture clears the screen and starts afresh.

The selected trace's label is inverted. Scope settings aren't saved with presets.

**Screensaver.** The scope keeps capturing on every page and is the app's screensaver: the same mode and zoom, without labels or status line, across the full width. How long the module waits is the platform's **Screen Blank** setting in the Setup/About calibration routine.

**Tips**

- **A still picture of a pattern:** use **Trig** mode with the trigger on your bar or reset pulse (a channel at /4, for example): every sweep starts in the same place, so changes stand out.
- **Compare two signals closely:** press the left encoder twice for two large traces, then move the cursor to choose which two.
- **Count ratchets or check timing:** press the right encoder to freeze the display.
- **Pk or Sm:** at slow timebases **Pk** draws even a 1 ms trigger a whole column wide, so every hit shows but pulse widths look longer than they are. Use a faster timebase or **Sm** to judge widths.

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
- **Older presets.** Banks and the working patch from earlier versions load as before; their Mods take the defaults and their Resets are off. A bank is rewritten in the new format the next time one of its slots is saved.
- **Audio in presets.** Saving a slot also stores the audio applet setup; loading applies it unless **Audio** is Off on the Global page. Slots saved before this existed have no audio setup and leave audio alone.
- **The working patch.** Your current settings and bank number are kept with the platform's normal app save: long-press the right encoder on (X)AND(N)OR in the app menu. Preset slots are saved to their files at once when you press A.

**Tips**

- **Before switching off:** save to a slot with **A**. That is written at once; the working patch is only kept if you also do the app-menu save.
- **Variations:** save a patch, change one thing, and save it in the next slot. Pressing **B** on each in turn is a quick A/B comparison, and **Load on** can then step through them in time.
- **One bank per project:** banks load as soon as you turn the left encoder, so a bank per song or performance keeps its slots together.

## Patch examples

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

- **Tip:** give all four channels the same **Reset** input (or use **Rst all**) and the staircase always starts from the top on the bar.

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

- **Tip:** with no LFO to hand, set the Ratchet **CV** to **Mod2** (Rand) instead: rolls come and go at random.

### Off-beats and swing from one clock

With the default patch running, set CH2 to **x1**, **Delay** to half the clock period, and its Length to 10 ms: output B now fires on the off-beat. Patch a slow LFO into CH2's Delay **CV** for a moving swing feel, and route **E** to **OR 1 2** for a combined on-and-off-beat pattern.

- **Tip:** the delay is a time, so it stays put if the tempo changes. Re-set it to half the new clock period after a tempo change.

### Driving the audio applets

Each audio applet picks its trigger and CV inputs from the platform's source list, which includes (X)AND(N)OR's outputs. On the Audio page they keep the platform's names, **#A**–**#H**, rather than OutA–OutH. The Mods and the channel sources aren't in that list; to give an applet a Mod, set an output to **MOD** and pick that output.

- **A drum on every clock:** load **FM Drum** into an audio slot and set its trigger to **#E**. In the default patch E is CH1 XOR CH2, the slice from 10 ms to 50 ms after each clock, so the drum fires once per clock. For a sparser pattern, route E to **OUT 2** and lower CH2's **Prob**.
- **Envelope filter:** set output H to **ENV** (AD, 2 ms attack, 300 ms decay) on CH1, and set a filter applet's cutoff CV (Ladder or FilterFolder, for example) to **#H**. Each CH1 gate sweeps the filter.
- **Internal melody:** set output G to **BIN** with **Semi** steps (see [Using BIN](#using-bin)), and set an oscillator applet's pitch CV (HarmOsc, for example) to **#G**. The channel pattern plays as notes.
- **Rhythmic delay:** use a **CNT** output with **1/3V** steps as a delay applet's time or feedback CV. The delay follows how many channels are high.
