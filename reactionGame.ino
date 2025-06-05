#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize LCD (address 0x27 is common, adjust if needed)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
const int button1Pin = 2;  // Player 1 button (left, NO terminal)
const int button2Pin = 4;  // Player 2 button (right, NO terminal)
const int ledPins[] = {7, 8, 9};  // 3 LEDs (LED1 pin 7, LED2 pin 8, LED3 pin 9)
const int numLeds = 3;

// Game variables
int player1Score = 0;
int player2Score = 0;
int roundCount = 0;
const int maxRounds = 5;
bool gameActive = false;

// Debouncing variables
const unsigned long debounceDelay = 50;  // 50ms debounce time
bool lastButton1State = LOW;
bool lastButton2State = LOW;
bool button1Pressed = false;
bool button2Pressed = false;

// Serial print timing
unsigned long lastSerialPrint = 0;
const unsigned long serialPrintInterval = 100;  // Print every 100ms

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Reaction Game Started");

  // Initialize pins
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);  // LEDs off initially
  }

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  updateDisplay("Start Game");
  lcd.setCursor(0, 1);
  lcd.print("Press both btns");
  Serial.println("LCD Initialized: Start Game, Press both btns");
}

void loop() {
  // Read button states with debouncing
  bool button1State = debounceButton(button1Pin, lastButton1State, button1Pressed);
  bool button2State = debounceButton(button2Pin, lastButton2State, button2Pressed);

  // Print button states in the main loop (waiting for game start)
  if (millis() - lastSerialPrint >= serialPrintInterval) {
    Serial.print("Main Loop - P1: ");
    Serial.print(button1State ? "1" : "0");
    Serial.print(" | P2: ");
    Serial.print(button2State ? "1" : "0");
    Serial.println();
    lastSerialPrint = millis();
  }

  // Wait for both buttons to be pressed to start the game
  if (!gameActive && button1State && button2State) {
    gameActive = true;
    Serial.println("Both buttons pressed - Starting game");
    // Reset button states and wait for release
    button1Pressed = false;
    button2Pressed = false;
    lastButton1State = LOW;
    lastButton2State = LOW;
    // Wait for buttons to be released to avoid immediate early press
    while (digitalRead(button1Pin) == HIGH || digitalRead(button2Pin) == HIGH) {
      delay(10);
    }
    startNewGame();
  }
}

bool debounceButton(int pin, bool &lastState, bool &pressedFlag) {
  bool reading = digitalRead(pin);
  bool state = lastState;

  // If the button state has changed, wait for debounce
  if (reading != lastState) {
    delay(debounceDelay);
    reading = digitalRead(pin);
    if (reading != lastState) {
      state = reading;
      // Only set pressed flag on rising edge (LOW to HIGH)
      if (state == HIGH) {
        pressedFlag = true;
      }
    }
  }
  
  lastState = state;
  return pressedFlag;
}

void startNewGame() {
  player1Score = 0;
  player2Score = 0;
  roundCount = 0;
  // Ensure button states are reset before starting
  button1Pressed = false;
  button2Pressed = false;
  lastButton1State = LOW;
  lastButton2State = LOW;
  playRound();
}

void playRound() {
  if (roundCount >= maxRounds) {
    endGame();
    return;
  }

  roundCount++;
  updateDisplay("Round " + String(roundCount));
  displayScores();
  Serial.println("Round " + String(roundCount) + " started");

  // Turn on all LEDs
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], HIGH);
  }

  // Countdown: Turn off LEDs one by one
  for (int i = 0; i < numLeds; i++) {
    unsigned long startTime = millis();
    unsigned long delayTime = random(500, 2001);  // Random delay between 0.5-2 seconds
    while (millis() - startTime < delayTime) {
      // Check button states during countdown
      bool button1State = debounceButton(button1Pin, lastButton1State, button1Pressed);
      bool button2State = debounceButton(button2Pin, lastButton2State, button2Pressed);

      // Print button states during countdown
      if (millis() - lastSerialPrint >= serialPrintInterval) {
        Serial.print("Countdown LED " + String(i + 1) + " - P1: ");
        Serial.print(button1State ? "1" : "0");
        Serial.print(" | P2: ");
        Serial.print(button2State ? "1" : "0");
        Serial.println();
        lastSerialPrint = millis();
      }

      if (button1State) {
        player2Score++;
        updateDisplay("Player 1 Early");
        displayScores();
        Serial.println("Player 1 pressed too early - P2 wins round");
        delay(2000);
        button1Pressed = false;
        button2Pressed = false;
        playRound();
        return;
      }
      if (button2State) {
        player1Score++;
        updateDisplay("Player 2 Early");
        displayScores();
        Serial.println("Player 2 pressed too early - P1 wins round");
        delay(2000);
        button1Pressed = false;
        button2Pressed = false;
        playRound();
        return;
      }
    }
    digitalWrite(ledPins[i], LOW);
    Serial.println("LED " + String(i + 1) + " turned off");
  }

  // Reaction time measurement
  bool winnerDeclared = false;
  while (!winnerDeclared) {
    bool button1State = debounceButton(button1Pin, lastButton1State, button1Pressed);
    bool button2State = debounceButton(button2Pin, lastButton2State, button2Pressed);

    // Print button states during reaction phase
    if (millis() - lastSerialPrint >= serialPrintInterval) {
      Serial.print("Reaction Phase - P1: ");
      Serial.print(button1State ? "1" : "0");
      Serial.print(" | P2: ");
      Serial.print(button2State ? "1" : "0");
      Serial.println();
      lastSerialPrint = millis();
    }

    if (button1State) {
      player1Score++;
      updateDisplay("Player 1 Won");
      displayScores();
      Serial.println("Player 1 wins round " + String(roundCount));
      winnerDeclared = true;
    }
    else if (button2State) {
      player2Score++;
      updateDisplay("Player 2 Won");
      displayScores();
      Serial.println("Player 2 wins round " + String(roundCount));
      winnerDeclared = true;
    }
  }

  delay(2000);  // Show winner for 2 seconds
  button1Pressed = false;
  button2Pressed = false;
  playRound();
}

void updateDisplay(String message) {
  // Reinitialize LCD to ensure it's responsive
  lcd.init();
  lcd.backlight();
  lcd.clear();
  // Center-align the message on the first line (16 characters wide)
  int padding = (16 - message.length()) / 2;
  lcd.setCursor(padding, 0);
  lcd.print(message);
  Serial.println("LCD Update: " + message);
}

void displayScores() {
  lcd.setCursor(0, 1);
  lcd.print("P1:");
  lcd.print(player1Score);
  lcd.setCursor(8, 1);
  lcd.print("P2:");
  lcd.print(player2Score);
  Serial.println("LCD Scores: P1:" + String(player1Score) + " P2:" + String(player2Score));
}

void endGame() {
  updateDisplay("End Game");
  displayScores();
  Serial.println("Game Over - " + String(player1Score > player2Score ? "Player 1 wins!" : player2Score > player1Score ? "Player 2 wins!" : "It's a tie!"));
  delay(10000);  // Show final result for 10 seconds
  updateDisplay("Start Game");
  lcd.setCursor(0, 1);
  lcd.print("Press both btns");
  Serial.println("LCD Reset: Start Game, Press both btns");
  gameActive = false;
  button1Pressed = false;
  button2Pressed = false;
  lastButton1State = LOW;
  lastButton2State = LOW;
  Serial.println("Game reset - Press both buttons to start again");
}