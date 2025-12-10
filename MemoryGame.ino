// ---------------------------
// RGB LED pin assignments
// ---------------------------
int ledPins[3][3] = {
  {2, 3, 4},    // LED 1: R,G,B
  {5, 6, 7},    // LED 2: R,G,B
  {8, 9, 10}    // LED 3: R,G,B
};

// ---------------------------
// Button pins (A3, A4, A5)
// Wiring for INPUT_PULLUP:
// pin ---- button ---- GND
// => idle = HIGH, pressed = LOW
// ---------------------------
int buttonPins[3] = {A3, A4, A5}; // Button 1, Button 2, Button 3

int motorPin1 = 11;
int motorPin2 = 12;

// ---------------------------
// Button state tracking
// ---------------------------
unsigned long lastPressTime[3] = {0, 0, 0};
int  clickCount[3]             = {0, 0, 0};
bool lastState[3]              = {HIGH, HIGH, HIGH};

const unsigned long debounceTime  = 50;   // ms
const unsigned long multiClickGap = 300;  // ms

// Ignore button noise right after reset
unsigned long startupIgnoreUntil = 0;     // NEW

// ---------------------------
// Game settings
// ---------------------------
// Max pattern length = 7 (hard). Easy=3, Medium=5, Hard=7.
const int MAX_PATTERN_LEN = 7;
int currentPatternLen = 5;  // will be set by difficulty

byte pattern[MAX_PATTERN_LEN];    // each step encodes (LED, colour)
byte userPattern[MAX_PATTERN_LEN];

int score = 0;
int roundsPlayed = 0;            // count completed rounds

// ---------------------------
// Pattern display timing
// ---------------------------
unsigned long patternLastChange = 0;
const unsigned long patternOnTime  = 800; // ms each LED lit
const unsigned long patternOffTime = 500; // ms gap between steps

int  patternIndexShow = 0;
bool patternLEDOn     = false;

// ---------------------------
// User input tracking
// ---------------------------
int userIndex = 0;

// ---------------------------
// Game state
// ---------------------------
enum GameState {
  SELECT_DIFFICULTY,   // choose Easy/Medium/Hard
  WAIT_FOR_START,      // "click any button to start round"
  SHOW_PATTERN,        // LEDs showing pattern
  WAIT_FOR_INPUT,      // user repeating pattern
  ASK_CONTINUE,        // ask if they want to keep playing
  GAME_OVER            // game ended
};

GameState gameState = SELECT_DIFFICULTY;

// ---------------------------
// Helper functions
// ---------------------------
void allLedsOff() {
  for (int i = 0; i < 3; i++) {
    for (int c = 0; c < 3; c++) {
      digitalWrite(ledPins[i][c], LOW);
    }
  }
}

void decodeStep(byte step, int &ledIndex, int &colorIndex) {
  // step 0..8  -> ledIndex = 0..2, colorIndex = 0..2
  ledIndex   = step / 3;
  colorIndex = step % 3;
}

const char* colorName(int colorIndex) {
  switch (colorIndex) {
    case 0: return "RED";
    case 1: return "GREEN";
    case 2: return "BLUE";
    default: return "?";
  }
}

void showStep(byte step) {
  int ledIndex, colorIndex;
  decodeStep(step, ledIndex, colorIndex);

  allLedsOff();
  digitalWrite(ledPins[ledIndex][colorIndex], HIGH);
}

void printStep(byte step) {
  int ledIndex, colorIndex;
  decodeStep(step, ledIndex, colorIndex);

  Serial.print("LED ");
  Serial.print(ledIndex + 1);
  Serial.print(" - ");
  Serial.print(colorName(colorIndex));
}

// Show difficulty menu (called at startup and when continuing)
void showDifficultyMenu() {
  Serial.println();
  Serial.println("Select difficulty:");
  Serial.println("  Button 1 = Easy   (3-step pattern)");
  Serial.println("  Button 2 = Medium (5-step pattern)");
  Serial.println("  Button 3 = Hard   (7-step pattern)");
  gameState = SELECT_DIFFICULTY;
}

// --------- PATTERN GENERATION (no immediate repeats) ----------
void generatePattern() {
  byte lastStep = 255; // impossible starting value

  for (int i = 0; i < currentPatternLen; i++) {
    byte step;
    do {
      step = random(0, 9);  // 0..8 (3 LEDs * 3 colours)
    } while (step == lastStep);  // avoid same step twice in a row

    pattern[i] = step;
    lastStep   = step;
  }
}

// Actually start a new round (after difficulty chosen and "start" clicked)
void startNewRound() {
  generatePattern();
  gameState         = SHOW_PATTERN;
  patternIndexShow  = 0;
  patternLEDOn      = false;
  patternLastChange = millis();
  userIndex         = 0;

  Serial.println();
  Serial.print("=== New Round ===  Length: ");
  Serial.print(currentPatternLen);
  Serial.print(" | Score: ");
  Serial.println(score);
  Serial.println("Watch the pattern...");
}

// After user enters full pattern, check it
void checkUserPattern() {
  roundsPlayed++;   // increment rounds played

  bool correct = true;
  for (int i = 0; i < currentPatternLen; i++) {
    if (userPattern[i] != pattern[i]) {
      correct = false;
      break;
    }
  }

  if (correct) {
    score++;
    Serial.println(">>> Correct pattern! 🎉");
    ////MOTOR STUFF!!
    if (score%3==0) {
      Serial.println(">>> Congratulations on getting 3 points!! Yay !!");
          // Forward at speed 40
      analogWrite(motorPin1, 100);
      analogWrite(motorPin2, 0);
      delay(1200);

      // Stop
      analogWrite(motorPin1, 0);
      analogWrite(motorPin2, 0);
      delay(3200);

      // Backward at speed 40
      analogWrite(motorPin1, 0);
      analogWrite(motorPin2, 100);
      delay(1200);

      // Stop
      analogWrite(motorPin1, 0);
      analogWrite(motorPin2, 0);
      delay(1200);
    }
  } else {
    Serial.println(">>> Incorrect pattern. ❌");
    Serial.print("Correct sequence was: ");
    for (int i = 0; i < currentPatternLen; i++) {
      printStep(pattern[i]);
      if (i < currentPatternLen - 1) Serial.print("  |  ");
    }
    Serial.println();
  }

  Serial.print("Score: ");
  Serial.println(score);
  Serial.print("Rounds played: ");
  Serial.println(roundsPlayed);
  Serial.println();

  // Ask if they want to keep playing
  Serial.println("Do you want to keep playing?");
  Serial.println("  Press Button 1 to continue.");
  Serial.println("  Press Button 2 or 3 to quit.");
  gameState = ASK_CONTINUE;
}

// Handle a completed multi-click on a given button
void handleCompletedClick(int buttonIndex, int clicks) {
  if (clicks < 1 || clicks > 3) {
    return; // ignore weird click counts
  }

  // If game is over, ignore further input
  if (gameState == GAME_OVER) {
    Serial.println("Game is over. Reset the board to play again.");
    return;
  }

  // 1) ASK_CONTINUE: yes/no after each round
  if (gameState == ASK_CONTINUE) {
    if (buttonIndex == 0) { // Button 1 => continue
      Serial.println("Continuing game...");
      showDifficultyMenu(); // choose difficulty for the next round
    } else { // Button 2 or Button 3 => quit
      Serial.println();
      Serial.println("Game over.");
      Serial.print("Final score: ");
      Serial.println(score);
      Serial.print("Total rounds played: ");
      Serial.println(roundsPlayed);
      Serial.println("These stats will be sent to the website (in the real system).");
      Serial.println("Reset the board if you want to start a new game.");
      gameState = GAME_OVER;
    }
    return;
  }

  // 2) SELECT_DIFFICULTY: choose pattern length, then ask to start
  if (gameState == SELECT_DIFFICULTY) {
    if (buttonIndex == 0) {
      currentPatternLen = 3; // Easy
      Serial.println("Difficulty selected: EASY (3-step pattern)");
    } else if (buttonIndex == 1) {
      currentPatternLen = 5; // Medium
      Serial.println("Difficulty selected: MEDIUM (5-step pattern)");
    } else if (buttonIndex == 2) {
      currentPatternLen = 7; // Hard
      Serial.println("Difficulty selected: HARD (7-step pattern)");
    } else {
      currentPatternLen = 5;
      Serial.println("Unknown button -> defaulting to MEDIUM (5-step pattern)");
    }

    Serial.println("Click any button to start this round.");
    gameState = WAIT_FOR_START;
    return;
  }

  // 3) WAIT_FOR_START: click any button to start showing pattern
  if (gameState == WAIT_FOR_START) {
    Serial.print("Button ");
    Serial.print(buttonIndex + 1);
    Serial.println(" pressed. Starting round...");
    startNewRound();
    return;
  }

  // 4) WAIT_FOR_INPUT: user entering the pattern
  if (gameState == WAIT_FOR_INPUT) {
    if (userIndex >= currentPatternLen) return;

    int ledIndex   = buttonIndex;    // 0->LED1, 1->LED2, 2->LED3
    int colorIndex = clicks - 1;     // 1 click->RED, 2->GREEN, 3->BLUE

    byte step = ledIndex * 3 + colorIndex;
    userPattern[userIndex] = step;
    userIndex++;

    Serial.print("Step ");
    Serial.print(userIndex);
    Serial.print(" input: LED ");
    Serial.print(ledIndex + 1);
    Serial.print(" - ");
    Serial.println(colorName(colorIndex));

    // Feedback flash
    showStep(step);
    delay(600);
    allLedsOff();

    if (userIndex >= currentPatternLen) {
      checkUserPattern();
    }
    return;
  }

  // 5) If pattern is currently showing (SHOW_PATTERN), ignore input.
}

// ---------------------------
// SETUP
// ---------------------------
void setup() {
  Serial.begin(9600);
  delay(500); // small delay so Serial Monitor can catch intro on some boards

  randomSeed(analogRead(A0));
  startupIgnoreUntil = millis() + 1000; // ignore buttons for first 1s

  // LED pins
  for (int i = 0; i < 3; i++) {
    for (int c = 0; c < 3; c++) {
      pinMode(ledPins[i][c], OUTPUT);
      digitalWrite(ledPins[i][c], LOW);
    }
  }

  // Buttons with INPUT_PULLUP
  // pin ---- button ---- GND
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastState[i]     = HIGH;  // idle = HIGH
    clickCount[i]    = 0;
    lastPressTime[i] = 0;
  }

  //setting motor off initially
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 0);

  Serial.println("RGB Memory Game Started!");
  Serial.println("Buttons during input: 1-click=RED, 2-click=GREEN, 3-click=BLUE.");
  Serial.println("Match the LED pattern exactly (LED + colour + order).");

  // Immediately show difficulty menu at startup
  showDifficultyMenu();
}

// ---------------------------
// PATTERN SHOW (non-blocking)
// ---------------------------
void runPatternShow() {
  if (gameState != SHOW_PATTERN) return;

  unsigned long now = millis();

  if (!patternLEDOn) {
    // LED currently OFF: wait for off-time, then turn next one ON
    if (now - patternLastChange >= patternOffTime) {
      if (patternIndexShow >= currentPatternLen) {
        // Done showing pattern
        allLedsOff();
        gameState = WAIT_FOR_INPUT;
        userIndex = 0;
        Serial.println("Your turn! Repeat the pattern using the buttons.");
        return;
      }

      showStep(pattern[patternIndexShow]);
      patternLEDOn      = true;
      patternLastChange = now;
    }
  } else {
    // LED is ON: wait for on-time, then turn OFF and move to next
    if (now - patternLastChange >= patternOnTime) {
      allLedsOff();
      patternLEDOn      = false;
      patternLastChange = now;
      patternIndexShow++;
    }
  }
}

// ---------------------------
// BUTTON READING (multi-click)
// ---------------------------
void readButtons() {
  unsigned long now = millis();

  for (int i = 0; i < 3; i++) {
    bool reading = digitalRead(buttonPins[i]);

    // Ignore all button logic during startup window
    if (now < startupIgnoreUntil) {
      lastState[i] = reading;
      continue;
    }

    // With INPUT_PULLUP: idle = HIGH, pressed = LOW
    if (reading == LOW && lastState[i] == HIGH) {
      if (now - lastPressTime[i] > debounceTime) {
        clickCount[i]++;
        lastPressTime[i] = now;
      }
    }

    // End of multi-click sequence
    if (clickCount[i] > 0 && (now - lastPressTime[i] > multiClickGap)) {
      int clicks = clickCount[i];
      clickCount[i] = 0;

      handleCompletedClick(i, clicks);
    }

    lastState[i] = reading;
  }
}

// ---------------------------
// MAIN LOOP
// ---------------------------
void loop() {
  runPatternShow();
  readButtons();
}