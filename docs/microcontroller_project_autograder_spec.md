# Microcontroller Project Autograder Specifications

*Author: Tarang Janawalkar (July 17, 2024)*

## Contents

1. [Terminology](#terminology)
2. [Program Emulation](#program-emulation)
   1. [The Tests](#the-tests)
   2. [The Events](#the-events)
   3. [Dealing with Timing Errors](#dealing-with-timing-errors)
3. [Emulator Inputs](#emulator-inputs)
   1. [Potentiometer Updates](#potentiometer-updates)
   2. [Pushbutton Inputs](#pushbutton-inputs)
   3. [UART Inputs](#uart-inputs)
4. [Emulator Outputs](#emulator-outputs)
   1. [Buzzer Output](#buzzer-output)
   2. [Display Output](#display-output)
   3. [UART Output](#uart-output)

## Terminology

The following terms are used throughout this document:

- **Test** – one execution of the emulator. Note that a single Gradescope test case may bundle multiple emulator runs.
- **Test output** – the feedback text captured for each test on Gradescope.
- **Input event** – an emulator stimulus that adjusts the potentiometer, toggles a pushbutton, or transmits a UART character to the ATtiny1626.
- **Gameplay input** – an input delivered to the Simon game. Each gameplay input is classified as either correct (matches Simon) or incorrect (fails to match Simon).
- **Output event** – a buzzer, display, or UART transmission detected by the emulator.
- **Event** – a group of buzzer and display output events originating from either Simon or the user.

## Program Emulation

The autograder drives an emulator that injects input events and records all output events produced by the program under test.

### The Tests

Sixteen emulator runs are executed. Some Gradescope test groups contain multiple emulator runs with differing parameters, timeouts, and stimulus sets. The nominal timeouts are:

1. Gameplay Test – 5505 ms
2. Pushbutton Hold Test – 7405 ms
3. Pushbutton Debouncing Test – 620 ms
4. Sequencing Test – 31 300 ms
5. Playback Delay Test – 4737.5 ms, 8237.5 ms, 11 737.5 ms, 15 237.5 ms
6. Playback Frequency Test – 9550 ms
7. UART Gameplay Test – 5630 ms
8. UART Octave Test – 2500 ms (twice)
9. High Score Test – 30 700 ms
10. Reset Test – 2555 ms
11. Seed Test – 5500 ms
12. Mixed Input Test – 12 710 ms

Efficient implementations must satisfy the expectations of each run before its timeout elapses.

### The Events

Each emulator run applies predetermined inputs that expect particular outputs. Output start times and durations are compared with a reference implementation that receives identical inputs. A condensed example of event logging is shown below:

```
[EVENT 1- Simon] [PLAYBACK DELAY: 250 ms]
PASS. Buzzer produced a 358.10 Hz tone at 0 ms for a duration of 125 ms.
PASS. Display produced "| " at 0 ms for a duration of 125 ms.

[EVENT 2- User Input: (S1 (15 ms))]
PASS. Buzzer produced a 358.10 Hz tone at 350 ms for a duration of 125 ms.
PASS. Display produced "| " at 350 ms for a duration of 125 ms.

[INPUT SEQUENCE CORRECT]
PASS. Display produced "88" at 500 ms for a duration of 250 ms.
```

Different tests may emit more or less detail depending on their objectives.

### Dealing with Timing Errors

Small timing tolerances are permitted, and short delays precede all gameplay inputs. The emulator rejects:

- Buzzer or display events whose durations differ from the reference by more than ±15 %.
- Buzzer or display events that begin before the corresponding gameplay input is issued.

Some durations depend on future outputs. For instance, if a program never displays the user score, the emulator may be unable to infer the length of the `FAIL` pattern once a mistake is made.

## Emulator Inputs

All valid inputs are listed beside their associated events in the test logs.

### Potentiometer Updates

Changing the potentiometer alters the playback delay. Logs highlight the active delay:

```
[EVENT 1- Simon] [PLAYBACK DELAY: 250 ms]
```

This delay remains in effect until another update occurs.

### Pushbutton Inputs

Pushbutton presses emit buzzer and display feedback when held long enough to overcome switch bounce. Logs describe the simulated presses and their durations:

```
// S1 pressed for 15 ms
[EVENT 2- User Input: (S1 (15 ms))]

// S1 pressed for 15 ms and S3 pressed for 15 ms
[EVENT 4- User Input: (S1 (15 ms), S3 (15 ms))]
```

For events with multiple inputs, subsequent buttons are pressed after the playback delay plus a short transition interval.

### UART Inputs

UART stimulus is displayed for UART-focused tests, including the delivery time of each character. Example log excerpt:

```
UART input:
250 ms: 'k'
350 ms: 'q'
1350 ms: ','
1450 ms: '1'
1700 ms: '3'
```

UART functions are summarised alongside the relevant events, for example:

```
[EVENT 2- User Input: ('q')]
[EVENT 4- User Input: ('1', 'e')]
```

Comments indicate the gameplay action triggered by each input (`S1`, `RESET`, `SEED`, octave changes, etc.). To accommodate alternate key mappings, some UART inputs are randomised.

## Emulator Outputs

Buzzer and display activity is logged when output tracing is enabled. A sample excerpt is shown below:

```
Detected buzzer events:
0 ms: 358.10 Hz, 50.00 % duty cycle
125 ms: 0.00 Hz, 0.00 % duty cycle
300 ms: 358.10 Hz, 50.00 % duty cycle
425 ms: 0.00 Hz, 0.00 % duty cycle
...

Detected display events:
0 ms: "| "
125 ms: "  "
300 ms: "| "
425 ms: "  "
...
```

UART transmissions captured before timeout are listed at the end of the log, for example:

```
UART output:
SUCCESS
1
GAME OVER
2
Enter name:
Patrick 2
...
```

### Buzzer Output

The autograder validates the start time, frequency, duty cycle, and duration of each tone. Durations are measured from the first occurrence of the expected tone (`t_on`) to the subsequent `0 Hz / 0 %` event (`t_off`). If a tone is emitted at an incorrect frequency, the emulator skips it until a matching tone appears, which can cause misaligned buzzer and display reports. Should a tone never stop (no `0 Hz` event within 50 % of the playback delay), the test halts at that event.

### Display Output

Display validation mirrors the buzzer rules, checking start times and durations. Because the display is multiplexed, patterns that illuminate both digits briefly exhibit transient outputs before stabilising. Proper multiplexing yields transient left/right digit updates that settle into a steady-state pattern; incorrect multiplexing prevents the emulator from detecting a steady state.

### UART Output

UART output is captured only once emulation finishes. Tests therefore look strictly at the produced text. Formatting must be exact—debug prints or deviations can substantially slow the program or cause failures.
