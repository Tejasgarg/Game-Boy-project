#include <FastLED.h>
#include <LinkedList.h>
#define NUM_LEDS    70
#define LED_PIN     12
int rows = 7;
int columns = 10;
CRGB leds[NUM_LEDS];

#define PAUSE_PIN 50
const int xPin        = A2;
const int yPin        = A1;

#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

int resetDelay = 250;

#define MAX_SPEED 5
#define MIN_SPEED 300
#define SPEED_LOSS 15
int gameSpeed = MIN_SPEED;
int currDirection = DIR_UP;
boolean isGamePaused = false;
boolean isTogglingPause = false;

class Point {
  private:
    byte x;
    byte y;
  public:
    Point(byte x, byte y) {
      this->x = x;
      this->y = y;
    }
    byte getX() {
      return x;
    }
    byte getY() {
      return y;
    }
    boolean isEqual(int x, int y) {
      return this->x == x && this->y == y;
    }
};

LinkedList<Point*> snakePositions = LinkedList<Point*>();

Point *applePosition;

CRGB appleColor = CRGB(255, 0, 0);
CRGB snakeColor = CRGB(0, 255, 0);
CRGB pausedAppleColor = CRGB(0, 255, 255);
CRGB pausedSnakeColor = CRGB(0, 0, 255);
CRGB emptyColor = CRGB(0, 0, 0);
CRGB solidColor = CRGB(255, 0, 0);

void setup() {
  Serial.begin(9600);

  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(PAUSE_PIN, INPUT_PULLUP); 
  
  snakePositions.add(getStartingPosition());
  
  applePosition = getApplePosition();

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
  checkForPause();
  
  if(isGamePaused) {
    pauseGame();
  }else{
    currDirection = getCurrentDirection();
  
    Point *nextPoint = getNextPosition();
  
    if(isNextPointValid(nextPoint)){
      playGame(nextPoint);
    }else{
      resetGame();
    }
  }
}

Point * getStartingPosition(){
  return new Point(0,0);
}

Point * getApplePosition(){
  int x = random(rows);
  int y = random(columns);

  while(snakeContainsPosition(x,y)){
    x = random(rows);
    y = random(columns);
  }
  return new Point(x,y);
}

boolean snakeContainsPosition(int x, int y) {
  for(int i = 0; i < snakePositions.size(); i++){
    if(snakePositions.get(i)->isEqual(x,y)) {
      return true;
    }
  }
  
  return false;
}

int getCurrentDirection(){
  int dir = currDirection;
  int xPosition = analogRead(xPin);
  int yPosition = analogRead(yPin);
  int mapX = map(xPosition, 0, 1023, -512, 512);
  int mapY = map(yPosition, 0, 1023, -512, 512);

  int absX = abs(mapX);
  int absY = abs(mapY);
  Serial.println(absX);
  int threshold = 384;
  if(absX > threshold || absY > threshold) {
    dir = absX > absY ? getXDir(mapX) : getYDir(mapY);
  }

  switch(dir) {
    case DIR_UP:
      dir = currDirection == DIR_DOWN ? DIR_DOWN : dir;
      break;
    case DIR_DOWN:
      dir = currDirection == DIR_UP ? DIR_UP : dir;
      break;
    case DIR_LEFT:
      dir = currDirection == DIR_RIGHT ? DIR_RIGHT : dir;
      break;
    case DIR_RIGHT:
      dir = currDirection == DIR_DOWN ? DIR_LEFT : dir;
      break;
    default:
      break;
  }

  return dir;
}

int getYDir(int y) {
  return y > 0 ? DIR_UP : DIR_DOWN;
}

int getXDir(int x) {
  return x > 0 ? DIR_RIGHT : DIR_LEFT;
}

Point * getHead() {
  return snakePositions.get(0);
}

Point * getTail() {
  return snakePositions.get(snakePositions.size() - 1);
}

void addToBeginning(Point *p){
  snakePositions.add(0, p);
}

void removeTail() {
  delete(snakePositions.pop());
}

Point * getNextPosition() {
  Point *head = getHead();
  switch(currDirection) {
    case DIR_UP:
      return new Point(head->getX(), head->getY() + 1);
    case DIR_DOWN:
      return new Point(head->getX(), head->getY() - 1);
    case DIR_LEFT:
      return new Point(head->getX() - 1, head->getY());
    case DIR_RIGHT:
      return new Point(head->getX() + 1, head->getY());
    default:
      return new Point(-9, -9);
  }
}

boolean isNextPointValid(Point *p) {
  int x = p->getX();
  int y = p->getY();

  if(x < 0 || x >= rows || y < 0 || y >= columns || snakeContainsPosition(x,y)) {
    return false;
  }

  return true;
}

void renderApple() {
  leds[getIndexForPoint(applePosition)] = isGamePaused ? pausedAppleColor : appleColor;
}

void renderSnake(){
  Point *p;
  for(int i = 0; i < snakePositions.size(); i++) {
    p = snakePositions.get(i);
    int index = getIndexForPoint(p);
    int x = p->getX();
    int y = p->getY();
    leds[index] = isGamePaused ? pausedSnakeColor : snakeColor;
  }
}

int getIndexForPoint(Point *p) {
  int x = p->getX();
  int y = p->getY();
  boolean oddRow = x % 2 == 1;

  if(oddRow){
    return (x + 1) * columns - y - 1;
  }
  
  return x * columns  + y;
}

void renderEmptyScreen(){
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = emptyColor;
  }
}

void renderSolidScreen() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = solidColor;
  }
}

void playGame(Point *nextPoint){

  renderEmptyScreen();
  
  if(applePosition->isEqual(nextPoint->getX(), nextPoint->getY())){
    growSnake(nextPoint);
  }else{
    moveSnake(nextPoint);
  }

  renderSnake();
  renderApple();
  
  FastLED.show();

  delay(gameSpeed);
}

void moveSnake(Point *p){
  addToBeginning(p);
  removeTail();
}

void growSnake(Point *p){
  addToBeginning(p);
  resetApple();
  increaseSpeed();
}

void increaseSpeed() {
  gameSpeed -= SPEED_LOSS;
}

void checkForPause() {
  boolean isPressedDown = digitalRead(PAUSE_PIN) == 0;
  
  boolean shouldPause = false;
  if(isPressedDown) {
    isTogglingPause = true;
  }else{
    shouldPause = isTogglingPause;
    isTogglingPause = false;
  }

  if(shouldPause) {
    isGamePaused = !isGamePaused;
  }
}

void pauseGame(){
  renderSnake();
  renderApple();
  FastLED.show();
}

void resetSnake() {
  while(snakePositions.size() > 0){
    delete(snakePositions.pop());
  }
  snakePositions.add(getStartingPosition());
}

void resetApple() {
  delete(applePosition);
  applePosition = getApplePosition();
}

void resetGame(){
  resetSnake();
  resetApple();
  gameSpeed = MIN_SPEED;
  currDirection = DIR_UP;
  renderSolidScreen();
  FastLED.show();
  delay(resetDelay);
  renderEmptyScreen();
  FastLED.show();
  delay(resetDelay);
  renderSolidScreen();
  FastLED.show();
  delay(resetDelay);
  renderEmptyScreen();
  FastLED.show();
  delay(resetDelay);
  renderSolidScreen();
  FastLED.show();
  delay(resetDelay);
  renderEmptyScreen();
  FastLED.show();
}