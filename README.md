# Cognitive Delirium Memory Game (Arduino)

A Simon Says–style **RGB LED memory game** built on **Arduino Uno R4** to support
cognitive monitoring for hospital patients at risk of delirium.

The game flashes a sequence of **LED + colour** combinations. The player must
repeat the sequence using three buttons. Difficulty scales with pattern length,
and successful rounds reward the player by activating a motor (e.g., to raise a
small flag). Scores and rounds are printed to the Serial Monitor so they can be
logged or sent to an external interface.

> Front-end / logging interface (Patient Game Log):  
> https://reflex-1bit.github.io/ece198/

---

## Features

- **3 RGB LEDs + 3 buttons**
  - Each LED has 3 colours (Red, Green, Blue).
  - Each button corresponds to one LED.
  - 1 click = Red, 2 clicks = Green, 3 clicks = Blue.

- **Difficulty selection**
  - Button 1 → Easy (3-step pattern)  
  - Button 2 → Medium (5-step pattern)  
  - Button 3 → Hard (7-step pattern)

- **Non-blocking pattern display**
  - Uses `millis()` to time LED on/off intervals instead of `delay()`.
  - Patterns are generated with no immediate repeats to keep it engaging.

- **Scoring + motor reward**
  - Score increases when the full pattern is entered correctly.
  - Every multiple of 3 points triggers the motor to run forward and backward
    as a reward.
  - Score and rounds played are printed to the Serial Monitor for logging.

- **Game loop**
  - Show pattern → wait for player input → check correctness → update score →
    reward or show correct pattern on Serial Monitor → choose to continue or quit.

---

## Hardware

- **Board:** Arduino Uno R4 (or compatible)
- **LEDs:** 3 × RGB LEDs (with appropriate resistors)
  - Pins 2–4: LED 1 (R, G, B)  
  - Pins 5–7: LED 2 (R, G, B)  
  - Pins 8–10: LED 3 (R, G, B)
- **Buttons:** 3 × momentary push buttons
  - Wired to analog pins `A3`, `A4`, `A5` using `INPUT_PULLUP`
  - One side to pin, the other side to GND.
- **Motor:** 1 × DC motor driven via a motor driver/H-bridge
  - Controlled from digital pins `11` and `12` using `analogWrite(...)`.

Check your own wiring diagram from the lab to confirm exact hardware.

---

## How to Play (Quick Rules)

1. **Choose a difficulty**
   - At startup, choose:
     - Button 1 → Easy (3-step pattern)
     - Button 2 → Medium (5-step pattern)
     - Button 3 → Hard (7-step pattern)

2. **Start the round**
   - After choosing difficulty, click any button to begin.

3. **Watch the LED pattern**
   - The 3 RGB LEDs will light up one at a time.
   - Each step encodes:
     - Which LED (1, 2, or 3)
     - What colour (Red, Green, or Blue)

4. **Repeat the pattern using the buttons**
   - Button 1 → LED 1  
   - Button 2 → LED 2  
   - Button 3 → LED 3  
   - Number of clicks = colour:
     - 1 click → Red
     - 2 clicks → Green
     - 3 clicks → Blue

5. **Feedback & scoring**
   - If the entire sequence is correct:
     - Score increments.
     - If score is 3 or a multiple of 3, the motor runs forward and backward
       as a reward.
   - If you get it wrong:
     - The correct sequence is printed to the Serial Monitor.

6. **Continue or quit**
   - Button 1 after a round → play again (reselect difficulty).
   - Button 2 or 3 → quit (final score + rounds printed in Serial).

---

## Getting Started

1. Open `MemoryGame.ino` in **Arduino IDE**.
2. Select the correct board (e.g., **Arduino Uno R4**) and COM port.
3. Wire the hardware according to the pins above.
4. Upload the sketch.
5. Open the **Serial Monitor** at 9600 baud to see prompts and stats.

---

## Future Ideas

- Send score + rounds data to a PC app or web interface for patient logging.
- Tune timing and difficulty for different patient groups.
- Add persistent high scores or session summary logging to EEPROM or SD card.

