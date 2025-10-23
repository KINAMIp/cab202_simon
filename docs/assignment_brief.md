# CAB202 Microprocessors and Digital Systems – Assessment 2: Microcontroller Project

*Faculty of Engineering, Queensland University of Technology (QUT)*

## Task Summary

Implement the Simon game on the QUTy development board. The classic Simon game presents a growing sequence of tones, which the user must repeat using four illuminated pushbuttons. Correct responses extend the sequence; incorrect responses end the game.

## Hardware Overview

- **Pushbuttons:** Four pushbuttons correspond to the four coloured sections of the Simon game. Each button drives a matching LED segment on the two-digit, seven-segment display.
- **Display:** The left digit (Digit A) and right digit (Digit B) display patterns defined in Table&nbsp;1.
- **Potentiometer:** Adjusts the playback delay, influencing how quickly tones are replayed.
- **Buzzer:** Generates tones whose frequencies are mapped to pushbuttons via the tone table specific to each student number.

### Pushbutton, Segment, and Tone Mapping

| Pushbutton | Segment | Tone |
|------------|---------|------|
| S1         | Digit A, segment `|` (E/left) | Tone 1 |
| S2         | Digit A, segment `-` (F/top)  | Tone 2 |
| S3         | Digit B, segment `|` (C/right)| Tone 3 |
| S4         | Digit B, segment `_` (D/bottom)| Tone 4 |

> **Note:** Consult the assignment resources for the exact tone frequencies associated with your allocated student number. Frequencies are derived from the Linear Feedback Shift Register (LFSR) seed.

## Section A – Gameplay

1. **Sequence Generation:**
   - Initialise the Linear-Feedback Shift Register (LFSR) with the student-specific seed from the Assignment Specification Section&nbsp;A.
   - Generate pseudo-random steps by masking the LFSR output with `STATE_LFSR & 0x03` and incrementing each step by one to map to button IDs 1–4.
   - Store steps sequentially in memory; the current level equals the sequence length.

2. **Sequence Playback:**
   - Play back the stored sequence on the buzzer and illuminate the appropriate segments for each step.
   - Insert the playback delay between steps; the delay is derived from the potentiometer (Section&nbsp;C) and updated in real time.

3. **User Turn:**
   - Await user inputs via pushbuttons or UART commands. Compare each user response with the stored sequence.
   - Correct sequences increase the level (max 99). Display the new level on the seven-segment display. If the level exceeds 99, flash `88` followed by the two least-significant digits.
   - Incorrect responses play the `FAIL` pattern and reset the game state.

4. **Success Pattern:**
   - When the user completes a sequence, display the `SUCCESS` pattern by illuminating all segments on both digits.

5. **Fail Pattern:**
   - When the user makes a mistake, display the `FAIL` pattern by showing the `C` segment on each digit.

## Section B – Sequence Details

- `STATE_LFSR` must be seeded from SRAM during reset and preserved between games.
- During playback, maintain `STATE_STEP` as the index of the next sequence element to present.
- During user input, maintain `STATE_STEP` as the index of the next expected input. A mismatch triggers the fail condition.
- The game supports pauses between rounds; if no user input is received within the timeout, reset to the attract mode.

## Section C – Playback Delay

- Playback delay is a function of the potentiometer voltage.
- Acceptable delay range: 250 ms (minimum) to 1250 ms (maximum), quantised into 10 discrete steps.
- The emulator emphasises delay changes in the logs; ensure the code responds to updates immediately.

## Section D – Playback Frequency

- Buzzer playback frequencies derive from the tone table (Section&nbsp;B of the Assignment Task Specification).
- The default octave corresponds to the base frequencies. UART commands adjust the octave:
  - `INC FREQ` (`'w'` or `'W'`) doubles the frequency (up one octave).
  - `DEC FREQ` (`'q'` or `'Q'`) halves the frequency (down one octave).
  - Limits: minimum 1/2 octave below the base frequencies, maximum two octaves above.

## Section E – Gameplay via UART

- UART settings: 9600 baud, 8 data bits, no parity, 1 stop bit (8-N-1).
- Gameplay commands:
  - `'1'`, `'2'`, `'3'`, `'4'` (and lowercase `'e'`, `'r'`, `'f'`, `'g'` alternatives) simulate buttons S1–S4.
  - `'q'` / `'Q'` decrements the octave (frequency /2).
  - `'w'` / `'W'` increments the octave (frequency ×2).
  - `'k'` triggers the reset sequence.
  - `'s'` seeds the RNG with the most recently provided hexadecimal value (see below).
  - `'n'` begins name entry for high scores.

- UART outputs must strictly match the provided templates (see Section&nbsp;E of the Assignment Task Specification).
- Ensure carriage returns/newlines follow the QUT assignment conventions (`"\r\n"` by default).

## Section F – High Scores

- Maintain high scores in SRAM. Each entry comprises `name`, `score`, and newline.
- High score table persists between games until reset. Upon `RESET`, reload from SRAM.
- When displaying the high score table via UART, output entries sorted by descending score; if scores tie, preserve the order of entry.
- Ensure debug `printf` statements are removed in the final submission to avoid timing issues.

## Coding Standards and Best Practices

1. **Code Organisation:**
   - Modularise functionality into logical files (e.g., `buzzer.c`, `display.c`, `input.c`, `uart.c`).
   - Keep interrupt service routines short; perform complex logic outside ISR context.

2. **GPIO Configuration:**
   - Configure all used ports explicitly during initialisation.
   - Use descriptive macros or inline functions for port manipulation.

3. **Bitmaps and Group Configurations:**
   - Store repeated bitmaps and segment groups centrally (e.g., in `display.h`).
   - Use enumerated types and switch statements for state machines instead of raw numeric literals.

4. **Memory Efficiency:**
   - Store tone tables and constant strings in program memory (`PROGMEM`) where possible.
   - Avoid redundant SRAM writes; minimise EEPROM access during gameplay.

5. **Testing Tips:**
   - Disable or wrap debug outputs behind conditional compilation flags.
   - Use the emulator logs to verify timing, especially for playback delay and debounce handling.

## Deliverables

- Submit the complete source code (zipped) to Gradescope by 11:59 PM, Friday 24 October.
- Ensure the program meets the real-time constraints of the autograder.
- Include any additional debugging output only when necessary for Section&nbsp;E diagnostics.
