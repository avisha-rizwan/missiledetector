#include <Servo.h>
#include <NewPing.h>

// --- Pin Definitions ---
const int trigPin = 7;
const int echoPin = 6;
const int ledPin = 3;
const int buzzerPin = 2;
const int launcherPin = 9;
const int radarPin = 10;

// --- Configuration Parameters ---
const int detectionDistance = 15; // Max detection range (cm)
const int minDistance = 5;        // Min detection range (cm)
const int radarLeftLimit = 15;    // Radar sweep left boundary
const int radarRightLimit = 165;  // Radar sweep right boundary
const int readyAngle = 90;        // Default facing forward angle
const int sweepDelay = 30;        // Delay between radar steps (ms)
const int maxPingDistance = 200;  // Maximum distance for NewPing timeout (cm)

// --- Global Objects ---
Servo launcherServo;
Servo radarServo;
NewPing sonar(trigPin, echoPin, maxPingDistance);

// --- Radar Variables ---
int radarAngle = 15;
int radarDirection = 1;
unsigned long lastRadarMove = 0;

// --- Launcher State Machine ---
enum LauncherState { IDLE, AIMING, FIRING_DOWN, FIRING_UP, RETURNING };
LauncherState currentLauncherState = IDLE;
int launcherAngle = readyAngle;
int lockedAngle = readyAngle;
unsigned long lastLauncherMove = 0;
const int aimSpeedDelay = 10;
unsigned long stateWaitTimer = 0;

// --- Alert State Machine ---
bool alertActive = false;
int alertCount = 0;
unsigned long lastAlertToggle = 0;
bool alertLedState = false;

// --- Detection Variables ---
int detectConfirm = 0;
bool targetLocked = false;
unsigned long lastPingTime = 0;
const int pingInterval = 40; // Ping every 40ms to avoid echo overlaps

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  launcherServo.attach(launcherPin);
  radarServo.attach(radarPin);
  
  // Set initial positions
  launcherServo.write(readyAngle); 
  radarServo.write(radarLeftLimit);    
  
  Serial.begin(9600);
  Serial.println("SYSTEM READY (NON-BLOCKING MODE)");
}

void loop() {
  unsigned long currentMillis = millis();

  updateRadar(currentMillis);
  checkSonar(currentMillis);
  updateLauncher(currentMillis);
  updateAlert(currentMillis);
}

// ---------------------------------------------------------
// RADAR STATE MACHINE
// ---------------------------------------------------------
void updateRadar(unsigned long currentMillis) {
  // If target is locked, stop the radar to focus on the target
  // (Remove this if you want the radar to keep scanning while firing!)
  if (targetLocked) return;

  if (currentMillis - lastRadarMove >= sweepDelay) {
    lastRadarMove = currentMillis;
    
    radarAngle += radarDirection;
    if (radarAngle >= radarRightLimit) {
      radarAngle = radarRightLimit;
      radarDirection = -1;
    } else if (radarAngle <= radarLeftLimit) {
      radarAngle = radarLeftLimit;
      radarDirection = 1;
    }
    
    radarServo.write(radarAngle);
  }
}

// ---------------------------------------------------------
// SONAR POLLING
// ---------------------------------------------------------
void checkSonar(unsigned long currentMillis) {
  // Stop pinging new targets while currently engaged
  if (targetLocked) return;

  if (currentMillis - lastPingTime >= pingInterval) {
    lastPingTime = currentMillis;
    
    // ping_cm() returns 0 if out of range
    unsigned int distance = sonar.ping_cm();
    
    if (distance > 0 && distance >= minDistance && distance <= detectionDistance) {
      detectConfirm++;
      lockedAngle = radarAngle; 
    } else {
      detectConfirm = 0;
    }

    if (detectConfirm >= 2) {
      targetLocked = true;
      Serial.print("TARGET LOCKED AT ANGLE: ");
      Serial.println(lockedAngle);
      
      // Trigger Alert
      alertActive = true;
      alertCount = 0;
      alertLedState = false;
      lastAlertToggle = currentMillis;
      
      // Trigger Launcher Sequence
      currentLauncherState = AIMING;
      lastLauncherMove = currentMillis;
      stateWaitTimer = currentMillis;
    }
  }
}

// ---------------------------------------------------------
// LAUNCHER STATE MACHINE
// ---------------------------------------------------------
void updateLauncher(unsigned long currentMillis) {
  switch (currentLauncherState) {
    case IDLE:
      // Waiting for target, do nothing
      break;

    case AIMING:
      if (currentMillis - lastLauncherMove >= aimSpeedDelay) {
        lastLauncherMove = currentMillis;
        
        if (launcherAngle < lockedAngle) launcherAngle++;
        else if (launcherAngle > lockedAngle) launcherAngle--;
        
        launcherServo.write(launcherAngle);
        
        // Check if we arrived at the target angle
        if (launcherAngle == lockedAngle) {
          if (currentMillis - stateWaitTimer >= 800) { 
             currentLauncherState = FIRING_DOWN;
             stateWaitTimer = currentMillis;
          }
        } else {
           stateWaitTimer = currentMillis; // Reset pause timer while moving
        }
      }
      break;

    case FIRING_DOWN:
      if (currentMillis - lastLauncherMove >= aimSpeedDelay) {
        lastLauncherMove = currentMillis;
        
        launcherAngle--;
        launcherServo.write(launcherAngle);
        
        // Dipped down to 0 degrees
        if (launcherAngle <= 0) {
           if (currentMillis - stateWaitTimer >= 500) {
              currentLauncherState = FIRING_UP;
              stateWaitTimer = currentMillis;
           }
        } else {
           stateWaitTimer = currentMillis;
        }
      }
      break;
      
    case FIRING_UP:
      if (currentMillis - lastLauncherMove >= aimSpeedDelay) {
        lastLauncherMove = currentMillis;
        
        launcherAngle++;
        launcherServo.write(launcherAngle);
        
        // Reached target angle again
        if (launcherAngle >= lockedAngle) {
          if (currentMillis - stateWaitTimer >= 500) {
              currentLauncherState = RETURNING;
              stateWaitTimer = currentMillis;
          }
        } else {
           stateWaitTimer = currentMillis;
        }
      }
      break;

    case RETURNING:
      if (currentMillis - lastLauncherMove >= aimSpeedDelay) {
        lastLauncherMove = currentMillis;
        
        if (launcherAngle < readyAngle) launcherAngle++;
        else if (launcherAngle > readyAngle) launcherAngle--;
        
        launcherServo.write(launcherAngle);
        
        // Reached home base
        if (launcherAngle == readyAngle) {
           currentLauncherState = IDLE;
           targetLocked = false;
           detectConfirm = 0;
           Serial.println("READY AGAIN");
        }
      }
      break;
  }
}

// ---------------------------------------------------------
// ALERT STATE MACHINE
// ---------------------------------------------------------
void updateAlert(unsigned long currentMillis) {
  if (!alertActive) return;

  if (currentMillis - lastAlertToggle >= 150) {
    lastAlertToggle = currentMillis;
    alertLedState = !alertLedState;
    
    if (alertLedState) {
      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, 1500);
    } else {
      digitalWrite(ledPin, LOW);
      noTone(buzzerPin);
      alertCount++;
      
      // Stop alert after 3 flashes/beeps
      if (alertCount >= 3) {
        alertActive = false;
      }
    }
  }
}