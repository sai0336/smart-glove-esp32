#include <Wire.h>
#include <Preferences.h>
#include "BleMouse.h" 

// --- PIN DEFINITIONS (From Circuit Diagram) ---
#define BTN_1 19    // Left Click / Pan
#define BTN_2 18    // Right Click / Zoom
#define BTN_3 5     // Toggle Active / Orbit
#define BTN_MODE 23 // Switch Main Modes (Long Press)

// LEDs
#define LED_M1 13   // Mode 1 Indicator (Yellow)
#define LED_M2 12   // Mode 2 Indicator (Yellow)
#define LED_M3 27   // Mode 3 Indicator (Yellow)
#define LED_ERR 26  // Red (Error / Idle)
#define LED_FB 4    // Green (Active / Feedback)
#define LED_BT 2    // Onboard Blue (Connection Status)

#define MPU_ADDR 0x68

BleMouse bleMouse("Smart Glove Master", "DIY", 100);
Preferences preferences;

int currentMode = 1;       
unsigned long modeTimer = 0;
bool modeHeld = false;
int16_t ax, ay, az, gx, gy, gz;

// --- MODE 1 VARIABLES (Air Mouse) ---
unsigned long rightBtnTimer = 0;
bool rightBtnActive = false;
bool isScrolling = false;
#define MOUSE_SENSITIVITY 2
#define SLOW_DIVIDER 600
#define SCROLL_THRESHOLD 200
#define DEADZONE 2

// --- MODE 2 & 3 SHARED TOGGLE ---
bool isModeActive = false; // Tracks if we are "Active" (Green) or "Idle" (Red)
bool lastB3State = HIGH;

// --- MODE 2 VARIABLES (3D Lab) ---
bool isGrabbing = false;
#define DEADZONE_3D 6000
#define SPEED_3D 4

// --- MODE 3 VARIABLES (Map Explorer) ---
unsigned long b3Timer = 0;
bool b3HoldActive = false; // Orbit Logic
unsigned long b2Timer = 0;
bool b2HoldActive = false; // Zoom Logic
bool isDragging = false;
#define DEADZONE_MAP 4
#define CURSOR_SPEED 1.5
#define SCROLL_SPEED 1
#define HOLD_TIME 300

void feedback() {
  digitalWrite(LED_FB, HIGH); delay(50); digitalWrite(LED_FB, LOW);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BTN_1, INPUT_PULLUP); pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP); pinMode(BTN_MODE, INPUT_PULLUP);
  
  pinMode(LED_M1, OUTPUT); pinMode(LED_M2, OUTPUT);
  pinMode(LED_M3, OUTPUT); pinMode(LED_ERR, OUTPUT);
  pinMode(LED_FB, OUTPUT); pinMode(LED_BT, OUTPUT);

  preferences.begin("glove-os", false);
  currentMode = preferences.getInt("mode", 1);
  preferences.end();

  // Initialize Hardware
  Wire.begin(21, 22);
  if(currentMode == 2) Wire.setClock(400000); else Wire.setClock(100000);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0); Wire.endTransmission();
  
  // Start Bluetooth ONCE
  bleMouse.begin();
  Serial.println("System Started");
  
  // Initial State: Mode 1 is Active, Others start Idle
  isModeActive = (currentMode == 1);
  
  // Update LEDs immediately
  updateLEDs();
}

void loop() {
  checkModeSwitch(); 
  updateLEDs();

  if (bleMouse.isConnected()) {
    if (currentMode == 1) runAirMouse();
    else if (currentMode == 2) run3DLab();
    else if (currentMode == 3) runMapExplorer();
  }
  
  delay(10); 
}

void updateLEDs() {
  // 1. Mode Indicators (Yellow)
  digitalWrite(LED_M1, currentMode == 1);
  digitalWrite(LED_M2, currentMode == 2);
  digitalWrite(LED_M3, currentMode == 3);
  
  // 2. Bluetooth Connection (Onboard LED)
  bool connected = bleMouse.isConnected();
  digitalWrite(LED_BT, connected);

  // 3. Status LEDs (Red/Green)
  if (!connected) {
    // Blink Red if disconnected
    if ((millis() / 500) % 2 == 0) digitalWrite(LED_ERR, HIGH); else digitalWrite(LED_ERR, LOW);
    digitalWrite(LED_FB, LOW);
  } 
  else {
    if (currentMode == 1) {
       // Mode 1: Always ready, no Red LED
       digitalWrite(LED_ERR, LOW); 
       // Green LED only flashes on clicks (handled in logic), usually OFF
    } 
    else {
       // Mode 2 & 3: Toggle Logic
       if (isModeActive) {
         digitalWrite(LED_ERR, LOW);  // Red OFF
         digitalWrite(LED_FB, HIGH);  // Green ON
       } else {
         digitalWrite(LED_ERR, HIGH); // Red ON (Idle)
         digitalWrite(LED_FB, LOW);   // Green OFF
       }
    }
  }
}

void checkModeSwitch() {
  if (digitalRead(BTN_MODE) == LOW) {
    if (modeTimer == 0) modeTimer = millis();
    if (millis() - modeTimer > 1000 && !modeHeld) {
      modeHeld = true; feedback();
      
      // Cycle Mode
      currentMode++;
      if (currentMode > 3) currentMode = 1;
      
      // Save Preference
      preferences.begin("glove-os", false);
      preferences.putInt("mode", currentMode);
      preferences.end();
      
      // Reset Mode States
      isModeActive = (currentMode == 1); // Only Mode 1 auto-starts
      isGrabbing = false;
      isDragging = false;
      b2HoldActive = false;
      b3HoldActive = false;
      lastB3State = HIGH;
      
      // Adjust Sensor Speed
      if(currentMode == 2) Wire.setClock(400000); else Wire.setClock(100000);
      
      // Visual Confirm
      for(int i=0; i<3; i++) { 
        digitalWrite(LED_FB, HIGH); delay(50); 
        digitalWrite(LED_FB, LOW); delay(50); 
      }
      
    }
  } else { modeTimer = 0; modeHeld = false; }
}


// MODE 1: AIR MOUSE (Standard)

void runAirMouse() {
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x43); Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  if (Wire.available() >= 6) {
    gx = Wire.read()<<8|Wire.read();
    int temp = Wire.read()<<8|Wire.read(); 
    gz = Wire.read()<<8|Wire.read();
    
    int mx = -gz / SLOW_DIVIDER * MOUSE_SENSITIVITY;
    int my = -gx / SLOW_DIVIDER * MOUSE_SENSITIVITY;
    int scr = -gx / 600;

    if (abs(mx) < DEADZONE) mx = 0;
    if (abs(my) < DEADZONE) my = 0;
    
    bleMouse.move(mx, my);

    if (digitalRead(BTN_2) == LOW) {
      if (!rightBtnActive) { rightBtnActive = true; rightBtnTimer = millis(); }
      if ((millis() - rightBtnTimer) > SCROLL_THRESHOLD) isScrolling = true;
    } else {
      if (rightBtnActive) {
        if (!isScrolling) bleMouse.click(MOUSE_RIGHT);
        rightBtnActive = false; isScrolling = false;
      }
    }
    if (isScrolling && scr != 0) { bleMouse.move(0, 0, scr); delay(100); }
  }
  
  if (digitalRead(BTN_1) == LOW) { if(!bleMouse.isPressed(MOUSE_LEFT)) bleMouse.press(MOUSE_LEFT); }
  else { if(bleMouse.isPressed(MOUSE_LEFT)) bleMouse.release(MOUSE_LEFT); }
}


// MODE 2: 3D LAB 

void run3DLab() {
  // B3 Toggle: Activate/Deactivate
  int b3 = digitalRead(BTN_3);
  if (b3 == LOW && lastB3State == HIGH) {
    isModeActive = !isModeActive;
    feedback();
    if (!isModeActive && isGrabbing) { bleMouse.release(MOUSE_LEFT); isGrabbing = false; }
  }
  lastB3State = b3;

  if (isModeActive) {
    Wire.beginTransmission(MPU_ADDR); Wire.write(0x3B); Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 14, true);
    if (Wire.available() >= 14) {
      ax = Wire.read()<<8|Wire.read(); ay = Wire.read()<<8|Wire.read(); while(Wire.available()) Wire.read();
      
      int mx = 0, my = 0;
      if (ax > DEADZONE_3D) mx = -SPEED_3D; else if (ax < -DEADZONE_3D) mx = SPEED_3D;
      if (ay > DEADZONE_3D) my = SPEED_3D; else if (ay < -DEADZONE_3D) my = -SPEED_3D;
      
      if (mx != 0 || my != 0) {
        if (!isGrabbing) { bleMouse.press(MOUSE_LEFT); isGrabbing = true; }
        bleMouse.move(mx, my); delay(15);
      } else {
        if (isGrabbing) { bleMouse.release(MOUSE_LEFT); isGrabbing = false; }
      }
    }
    if (digitalRead(BTN_1) == LOW) { bleMouse.move(0, 0, 1); delay(100); }
    if (digitalRead(BTN_2) == LOW) { bleMouse.move(0, 0, -1); delay(100); }
  }
}


// MODE 3: MAP EXPLORER 

void runMapExplorer() {
  // Read Sensors first
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x43); Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  if (Wire.available() >= 6) {
    gx = Wire.read()<<8|Wire.read(); gy = Wire.read()<<8|Wire.read(); gz = Wire.read()<<8|Wire.read();
  }

  // --- B3 LOGIC: TAP = TOGGLE | HOLD = ORBIT ---
  int b3State = digitalRead(BTN_3);
  if (b3State == LOW) { // Pressed
    if (b3Timer == 0) b3Timer = millis();
    // If held longer than HOLD_TIME and Mode is Active -> Start Orbit
    else if ((millis() - b3Timer > HOLD_TIME) && !b3HoldActive && isModeActive) {
       bleMouse.press(MOUSE_MIDDLE); // Middle Click Hold
       b3HoldActive = true; 
    }
  } else if (b3State == HIGH && b3Timer > 0) { // Released
    if (!b3HoldActive) {
      // Short Press -> Toggle Mode Active/Idle
      isModeActive = !isModeActive;
      feedback();
    } else {
      // Long Press Release -> Stop Orbit
      bleMouse.release(MOUSE_MIDDLE);
      b3HoldActive = false;
    }
    b3Timer = 0;
  }

  // If Idle, do nothing else
  if (!isModeActive) return; 

  // --- B2 LOGIC: TAP = RIGHT CLICK | HOLD = ZOOM ---
  int b2State = digitalRead(BTN_2);
  if (b2State == LOW) {
    if (b2Timer == 0) b2Timer = millis();
    if (millis() - b2Timer > HOLD_TIME) b2HoldActive = true;
    
    if (b2HoldActive) { // Zooming based on tilt
      if (gx < -1500) { bleMouse.move(0,0, SCROLL_SPEED); delay(60); }
      else if (gx > 1500) { bleMouse.move(0,0, -SCROLL_SPEED); delay(60); }
      // Horizontal Scroll
      if (gz > 2000) { bleMouse.move(0,0,0, -SCROLL_SPEED); delay(100); }
      else if (gz < -2000) { bleMouse.move(0,0,0, SCROLL_SPEED); delay(100); }
    }
  } else if (b2State == HIGH && b2Timer > 0) {
    if (!b2HoldActive) bleMouse.click(MOUSE_RIGHT); // Tap
    b2Timer = 0; b2HoldActive = false;
  }

  // --- B1 LOGIC: TAP = LEFT CLICK | HOLD = PAN ---
  // Only allow Panning if NOT Zooming and NOT Orbiting
  if (!b2HoldActive && !b3HoldActive) {
    if (digitalRead(BTN_1) == LOW) {
      if (!isDragging) { bleMouse.press(MOUSE_LEFT); isDragging = true; }
    } else {
      if (isDragging) { bleMouse.release(MOUSE_LEFT); isDragging = false; }
    }
  }

  // --- MOVEMENT LOGIC ---
  // Move allowed if: Normal OR Panning OR Orbiting. (Not Zooming)
  if (!b2HoldActive) {
    int mx = -(gz / 600) * CURSOR_SPEED;
    int my = -(gx / 600) * CURSOR_SPEED;
    if (abs(mx) < DEADZONE_MAP) mx = 0;
    if (abs(my) < DEADZONE_MAP) my = 0;
    if (mx!=0 || my!=0) bleMouse.move(mx, my);
  }
}