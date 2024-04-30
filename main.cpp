/**
 * Reflex Rush! - A game to test your reflexes and hand-eye coordination
 * 
 * This code uses multiple parts of code from Jon Froehlich(@jonfroehlich), http://makeabilitylab.io, co-pilot and ChatGPT 3.5
 * https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino
 * https://github.com/makeabilitylab/arduino/blob/master/MakeabilityLab_Arduino_Library/src/Shape.hpp - requires Shape.hpp from the MakeabilityLab_Arduino_Library
 * 
 * Code originally based on:
 * https://makeabilitylab.github.io/p5js/Games/FlappyBird
 * https://makeabilitylab.github.io/p5js/Games/FlappyBird2
 *
 */

#include <Wire.h>
#include <Shape.hpp>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED _display width, in pixels
#define SCREEN_HEIGHT 64 // OLED _display height, in pixels

// Declaration for an SSD1306 _display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char TITLE_LINE1[] = "Reflex";
const char TITLE_LINE2[] = "Rush!";
const char PLAY_CONTROLS[] = "L R";
const char STR_GAME_OVER[] = "Too Slow!";
const char STR_START_GAME[] = "Press START";
// create an array of end games messages to say 
const char* STR_END_GAME_MESSAGES[] = {"Too Slow!", "Dont Stop!", "FASTER!"};
// chose a random message to display at the end of the game
int randomIndex = random(sizeof(STR_END_GAME_MESSAGES) / sizeof(STR_END_GAME_MESSAGES[0]));
// array of string difficulties
const char* STR_LOADSCREEN_DIFFICULTY[] = {"easy", "medium", "hard"};

// Define I/O pins
const int VIBROMOTOR_OUTPUT_PIN = 5;
const int START_BUTTON_INPUT_PIN = 9;
const int LEFT_OUTPUT_PIN = 10;
const int BUZZER_OUTPUT_PIN = 11;
const int RIGHT_OUTPUT_PIN = 12;
const int LED_OUTPUT_PIN = 13;

// const boolean _drawFrameCount = false; // change to show/hide frame count
const int DELAY_LOOP_MS = 5;
const int LOAD_SCREEN_SHOW_MS = 750;

// number of lives 
int _numLives = 3;
int _pipeSpeed = 1;
int _score = 0;
unsigned long _gameOverTimestamp = 0;

class Obstacle : public Circle {
  public:
    Obstacle(int x, int y, int width) : Circle(x, y, width)
    {
    }
};    

// Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
class Pipe : public Rectangle {
  protected:
    bool _hasPassedBALL = false;

  public:
    Pipe(int x, int y, int width, int height) : Rectangle(x, y, width, height)
    {
    }

    bool getHasPassedBALL() {
      return _hasPassedBALL;
    }

    bool setHasPassedBALL(bool hasPassedBALL) {
      _hasPassedBALL = hasPassedBALL;
    }
};

// Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
const int BALL_HEIGHT = 5;
const int BALL_RADIUS = 10;
const int NUM_PIPES = 7;
const int MIN_PIPE_WIDTH = 5;
const int MAX_PIPE_WIDTH = 10; // in pixels
const int MIN_PIPE_X_SPACING_DISTANCE = BALL_RADIUS * 3; // in pixels
const int MAX_PIPE_X_SPACING_DISTANCE = 30; // in pixels
const int MIN_PIPE_Y_SPACE = BALL_HEIGHT * 3;
const int MAX_PIPE_Y_SPACE = SCREEN_HEIGHT - BALL_HEIGHT * 2;

const int NUM_OBSTACLES = 3;
const int OBSTACLE_RADIUS = 4;
const int MIN_Obstacle_X_SPACING_DISTANCE = BALL_RADIUS * 3; // in pixels
const int MAX_Obstacle_X_SPACING_DISTANCE = 30; // in pixels
const int MIN_Obstacle_Y_SPACE = BALL_HEIGHT * 3;
const int MAX_Obstacle_Y_SPACE = SCREEN_HEIGHT - BALL_HEIGHT * 2;

const int IGNORE_INPUT_AFTER_GAME_OVER_MS = 500; //ignores input for 500ms after game over

// Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
// Initialize top pipe and bottom pipe arrays. The location/sizes don't matter
// at this point as we'll set them in setup()
Pipe _topPipes[NUM_PIPES] = { Pipe(0, 0, 0, 0),
                              Pipe(0, 0, 0, 0),
                              Pipe(0, 0, 0, 0),
                              Pipe(0, 0, 0, 0),
                              Pipe(0, 0, 0, 0),
                              Pipe(0, 0, 0, 0),
                              Pipe(0, 0, 0, 0)
                            };

Pipe _bottomPipes[NUM_PIPES] = { Pipe(0, 0, 0, 0),
                                 Pipe(0, 0, 0, 0),
                                 Pipe(0, 0, 0, 0),
                                  Pipe(0, 0, 0, 0),
                                  Pipe(0, 0, 0, 0),
                                  Pipe(0, 0, 0, 0),
                                  Pipe(0, 0, 0, 0)

                               };
                               
Obstacle _obstacles[NUM_OBSTACLES] = { Obstacle(0, 0, 0),
                                      Obstacle(0, 0, 0),
                                      Obstacle(0, 0, 0)
                                    };

// Code https://github.com/makeabilitylab/arduino/blob/master/MakeabilityLab_Arduino_Library/src/Shape.hpp and modified by me
// Initialize the ball
Ball _ball(5, SCREEN_HEIGHT / 2, BALL_RADIUS - 2 * _numLives);

// Game states 
enum GameState {
  NEW_GAME,
  PLAYING,
  GAME_OVER,
};

GameState _gameState = NEW_GAME;

// This is necessary for the game to work on the ESP32
// See: 
//  - https://github.com/espressif/arduino-esp32/issues/1734
//  - https://github.com/Bodmer/TFT_eSPI/issues/189
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

// Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
// Initialize the game entities and game components
void initializeGameEntities() {
  _score = 0;

  _ball.setY(_display.height() / 2 - _ball.getHeight() / 2);
  _ball.setDrawFill(true);

  const int minStartXPipeLocation = _display.width() / 2;
  int lastPipeX = minStartXPipeLocation;
  for (int i = 0; i < NUM_PIPES; i++) {

    int pipeX = lastPipeX + random(MIN_PIPE_X_SPACING_DISTANCE, MAX_PIPE_X_SPACING_DISTANCE);
    int pipeWidth = random(MIN_PIPE_WIDTH, MAX_PIPE_WIDTH);

    int yGapBetweenPipes = random(MIN_PIPE_Y_SPACE, MAX_PIPE_Y_SPACE);

    int topPipeY = 0;
    int topPipeHeight = random(0, SCREEN_HEIGHT - yGapBetweenPipes);

    int bottomPipeY = topPipeHeight + yGapBetweenPipes;
    int bottomPipeHeight = SCREEN_HEIGHT - bottomPipeY;

    _topPipes[i].setLocation(pipeX, topPipeY);
    _topPipes[i].setDimensions(pipeWidth, topPipeHeight);
    _topPipes[i].setDrawFill(false);

    _bottomPipes[i].setLocation(pipeX, bottomPipeY);
    _bottomPipes[i].setDimensions(pipeWidth, bottomPipeHeight);
    _topPipes[i].setDrawFill(false);


    lastPipeX = _topPipes[i].getRight();
  }

  // draw circles inside of the pipes 
  for (int i = 0; i < NUM_OBSTACLES; i++) {
    int obstacleX = lastPipeX + random(MIN_Obstacle_X_SPACING_DISTANCE, MAX_Obstacle_X_SPACING_DISTANCE);
    int obstacleY = random(MIN_Obstacle_Y_SPACE, MAX_Obstacle_Y_SPACE);

    _obstacles[i].setLocation(obstacleX, obstacleY);
    _obstacles[i].setRadius(OBSTACLE_RADIUS);
    _obstacles[i].setDrawFill(true);

    lastPipeX = _obstacles[i].getRight();
  }

}
// Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
// This function is called when the game is not in play mode
void nonGamePlayLoop(int randomIndex) {
  // Draw the pipes
  for (int i = 0; i < NUM_PIPES; i++) {
    _topPipes[i].draw(_display);
    _bottomPipes[i].draw(_display);
  }

  int16_t x1, y1;
  uint16_t w, h;
  int play = digitalRead(START_BUTTON_INPUT_PIN);
  if (_gameState == NEW_GAME) {
    if (play == LOW) {
      _gameState = PLAYING;
    }
  } else if (_gameState == GAME_OVER) {

    _display.setTextSize(2);
    _display.getTextBounds(STR_GAME_OVER, 0, 0, &x1, &y1, &w, &h);
    int yText = 15;
    _display.setCursor(_display.width() / 2 - w / 2, yText);
    _display.print(STR_END_GAME_MESSAGES[randomIndex]);

    // print start game message
    yText = yText + h + 2;
    _display.setTextSize(1);
    _display.getTextBounds(STR_START_GAME, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(_display.width() / 2 - w / 2, yText);
    _display.print(" Press START");

    // print score 
    yText = yText + h + 2;
    _display.setTextSize(1);
    _display.getTextBounds("Score: 0", 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(_display.width() / 2 - w / 2, yText);
    _display.print("Score:");
    _display.print(_score);

    // We ignore input a bit after game over so that user can see end game screen
    // and not accidentally start a new game
    if (play == LOW && millis() - _gameOverTimestamp >= IGNORE_INPUT_AFTER_GAME_OVER_MS) {
      // if the current state is game over, need to reset
      initializeGameEntities();
      _gameState = PLAYING;
    }
  }
  _ball.draw(_display);
}

// create a function to play title sound 
void playTitleSound() {
  // play title
  tone(BUZZER_OUTPUT_PIN, 1000, 150);
  delay(150);
  tone(BUZZER_OUTPUT_PIN, 1500, 150);
  delay(150);
  tone(BUZZER_OUTPUT_PIN, 2000, 150);
  delay(150);
  tone(BUZZER_OUTPUT_PIN, 2500, 150);
  delay(150);
  noTone(BUZZER_OUTPUT_PIN);
}

// Code created by github co-pilot and modified by me
// Plays the sound when the user runs out of lives
void playDeathSound() {
  // play death
  tone(BUZZER_OUTPUT_PIN, 1000, 100);
  delay(100);
  tone(BUZZER_OUTPUT_PIN, 500, 100);
  delay(100);
  tone(BUZZER_OUTPUT_PIN, 250, 100);
  delay(100);
  tone(BUZZER_OUTPUT_PIN, 125, 100);
  delay(100);
  tone(BUZZER_OUTPUT_PIN, 62, 100);
  delay(100);
  noTone(BUZZER_OUTPUT_PIN);
}

// Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
// Loop for playing state of the game
void gamePlayLoop() {
  // Brightness of the LED is based on the number of lives
  int brightness = map(_numLives, 0, 10, 0, 255); // Map _numLives to brightness levels (0-255)
  Serial.println(brightness);
  analogWrite(LED_OUTPUT_PIN, brightness - 25);

  // Move the ball right
  _ball.setY(_ball.getY());
  if (digitalRead(RIGHT_OUTPUT_PIN) == LOW) {
    _ball.setY(_ball.getY() - 4);
    _display.drawLine(_ball.getX(), _ball.getY(), _ball.getX(), _ball.getY() + 10, WHITE);
  }

  // Move the ball left
  if (digitalRead(LEFT_OUTPUT_PIN) == LOW) {
    _ball.setY(_ball.getY() + 4);
    _display.drawLine(_ball.getX(), _ball.getY(), _ball.getX(), _ball.getY() + 10, WHITE);
  }

  // Code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
  _ball.forceInside(0, 0, _display.width(), _display.height());

  // xMaxRight tracks the furthest right pixel of the furthest right pipe
  // which we will use to reposition pipes that go off the left part of screen
  int xMaxRight = 0;

  // Iterate through pipes and check for collisions and scoring
  for (int i = 0; i < NUM_PIPES; i++) {
    _display.fillCircle(_bottomPipes[i].getX() + random(3,7), _bottomPipes[i].getY() - 10, 2, WHITE);

    _topPipes[i].setX(_topPipes[i].getX() - _pipeSpeed);
    _bottomPipes[i].setX(_bottomPipes[i].getX() - _pipeSpeed);
    // _display.drawCircle(_bottomPipes[i].getX() + 3, _bottomPipes[i].getY(), OBSTACLE_RADIUS, WHITE);

    _topPipes[i].draw(_display);
    _bottomPipes[i].draw(_display);

    Serial.println(_topPipes[i].toString());

    // Check if the BALL passed by the pipe
    if (_topPipes[i].getRight() < _ball.getLeft()) {

      // If we're here, the BALL has passed the pipe. Check to see
      // if we've marked it as passed yet. If not, then increment the score!
      if (_topPipes[i].getHasPassedBALL() == false) {
        _score++;
        // if not mod of 10 play one sound, else play another
        if (_score % 10 == 0) {
          tone(BUZZER_OUTPUT_PIN, 2000, 100);
          delay(10);
          noTone(BUZZER_OUTPUT_PIN);
        } else {
          tone(BUZZER_OUTPUT_PIN, 1000, 100);
          delay(10);
          noTone(BUZZER_OUTPUT_PIN);
        }

        _topPipes[i].setHasPassedBALL(true);
        _bottomPipes[i].setHasPassedBALL(true);
      }
    }

    // xMaxRight is used to track future placements of pipes once
    // they go off the left part of the screen
    if (xMaxRight < _topPipes[i].getRight()) {
      xMaxRight = _topPipes[i].getRight();
    }

    // Check for collisions and end of game
    if (_topPipes[i].overlaps(_ball)) {
      _topPipes[i].setDrawFill(true);
      // decrement lives 
      _numLives--;
      // vibrate
      digitalWrite(VIBROMOTOR_OUTPUT_PIN, HIGH);
      delay(100);
      digitalWrite(VIBROMOTOR_OUTPUT_PIN, LOW);
      // if lives are 0, game over
      if (_numLives <= 0) {
        _gameState = GAME_OVER;
        _numLives = 3;
        playDeathSound();
        _gameOverTimestamp = millis();
      }

      // _gameState = GAME_OVER;
      // playDeathSound();
      // _gameOverTimestamp = millis();
    } else {
      _topPipes[i].setDrawFill(false);
    }

    if (_bottomPipes[i].overlaps(_ball)) {
      _bottomPipes[i].setDrawFill(true);
      _numLives--;
      digitalWrite(VIBROMOTOR_OUTPUT_PIN, HIGH);
      delay(100);
      digitalWrite(VIBROMOTOR_OUTPUT_PIN, LOW);
      // if lives are 0, game over
      if (_numLives <= 0) {
        _gameState = GAME_OVER;
        _numLives = 3;
        playDeathSound();
        _gameOverTimestamp = millis();
      }
    } else {
      _bottomPipes[i].setDrawFill(false);
    }
  }

  // Check for pipes that have gone off the screen to the left
  // and reset them to off the screen on the right
  xMaxRight = max(xMaxRight, _display.width());
  for (int i = 0; i < NUM_PIPES; i++) {
    if (_topPipes[i].getRight() < 0) {
      int pipeX = xMaxRight + random(MIN_PIPE_X_SPACING_DISTANCE, MAX_PIPE_X_SPACING_DISTANCE);
      int pipeWidth = random(MIN_PIPE_WIDTH, MAX_PIPE_WIDTH);

      int yGapBetweenPipes = random(MIN_PIPE_Y_SPACE, MAX_PIPE_Y_SPACE);

      int topPipeY = 0;
      int topPipeHeight = random(0, SCREEN_HEIGHT - yGapBetweenPipes);

      int bottomPipeY = topPipeHeight + yGapBetweenPipes;
      int bottomPipeHeight = SCREEN_HEIGHT - bottomPipeY;

      _topPipes[i].setLocation(pipeX, topPipeY);
      _topPipes[i].setDimensions(pipeWidth, topPipeHeight);
      _topPipes[i].setHasPassedBALL(false);

      _bottomPipes[i].setLocation(pipeX, bottomPipeY);
      _bottomPipes[i].setDimensions(pipeWidth, bottomPipeHeight);
      _bottomPipes[i].setHasPassedBALL(false);

      xMaxRight = _topPipes[i].getRight();
    }
  }
  _ball.draw(_display);
}

// Function created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
// Displays title and instructions on the OLED screen
void showLoadScreen() {
  // Clear the buffer
  _display.clearDisplay();

  _display.setTextSize(1);
  _display.setTextColor(WHITE, BLACK);

  int16_t x1, y1;
  uint16_t w, h;

  int yText = 10;
  _display.setTextSize(2);
  _display.getTextBounds(TITLE_LINE1, 0, 0, &x1, &y1, &w, &h);
  _display.setCursor(_display.width() / 2 - w / 2, yText);
  _display.print(TITLE_LINE1);

  _display.setTextSize(1);
  yText = yText + h + 1;
  _display.getTextBounds(TITLE_LINE2, 0, 0, &x1, &y1, &w, &h);
  _display.setCursor(_display.width() / 2 - w / 2, yText);
  _display.print(TITLE_LINE2);

  _display.getTextBounds("    easy medium hard", 0, 0, &x1, &y1, &w, &h);
  int newXPosition = _display.width() / 2 - w / 2; 
  _display.setCursor(newXPosition, yText + h + 10); // Set the cursor position
  _display.print(" easy medium hard");

  _display.display();
}

// Setups the pins and initializes the _display
void setup() {
  pinMode(START_BUTTON_INPUT_PIN, INPUT_PULLUP);
  pinMode(LEFT_OUTPUT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_OUTPUT_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER_OUTPUT_PIN, OUTPUT);
  pinMode(VIBROMOTOR_OUTPUT_PIN, OUTPUT);
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate _display voltage from 3.3V internally
  if (!_display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  showLoadScreen();
}

// Code created by co-pilot and modified by me
// Centers the text on the screen
void centerText(const char *text) {
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(WHITE);

    // Get the width and height of the text
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    // Calculate the center position for the text
    int16_t x = (_display.width() - w) / 2;
    int16_t y = (_display.height() - h) / 2;

    // Set the cursor to the center position and print the text
    _display.setCursor(x, y);
    _display.setTextSize(2);
    
    // Code created by co-pilot and modified by me
    // Blink the text 3 times
    for (int i = 0; i < 2; i++) {
        _display.getTextBounds(PLAY_CONTROLS, 0, 0, &x1, &y1, &w, &h);
        _display.setCursor(_display.width() / 2 - w / 2, 15);
        _display.print(PLAY_CONTROLS);
        _display.display(); 
        delay(500);
        _display.clearDisplay(); 
        _display.display(); 
        delay(500);
    }
    _display.display(); 
}

// Parts of code created by https://github.com/makeabilitylab/arduino/blob/master/OLED/FlappyBird/FlappyBird.ino and modified by me
// Loop checks game state and runs the game
void loop() {
  // if new game, then click button to load lives and start 
  if (_gameState == NEW_GAME) {
    if (digitalRead(START_BUTTON_INPUT_PIN) == LOW || digitalRead(LEFT_OUTPUT_PIN) == LOW || digitalRead(RIGHT_OUTPUT_PIN) == LOW) {
      if (digitalRead(START_BUTTON_INPUT_PIN) == LOW) {
        _numLives = 7;
        // pipe speed
        _pipeSpeed = 4;
      } else if (digitalRead(LEFT_OUTPUT_PIN) == LOW) {
        _numLives = 5;
        _pipeSpeed = 5;
      } else if (digitalRead(RIGHT_OUTPUT_PIN) == LOW) {
        _numLives = 3;
        _pipeSpeed = 7;
      }
      // display press to play message for a moment 
      centerText(PLAY_CONTROLS);
      playTitleSound();
      delay(500);
      
      initializeGameEntities();
      _display.clearDisplay();
      _gameState = PLAYING;
    }
  } else { 
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setCursor(0, _display.height() - 8);
    _display.print(_score);
  if (_gameState == NEW_GAME || _gameState == GAME_OVER) {
    nonGamePlayLoop(randomIndex);
  } else if (_gameState == PLAYING) {
    gamePlayLoop();
  }

  // Draw the display buffer to the screen
  _display.display();

  if (DELAY_LOOP_MS > 0) {
    delay(DELAY_LOOP_MS);
  }
  }
}
