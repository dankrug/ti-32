/*
  TI-Link PCB test for XIAO ESP32C3
  ---------------------------------
  Verifies the level shifter between the ESP32C3 and the PCB output (5V, GND, TIP, RING).
  Used Hardware: pcb "TI-32 v0.1" by Chromalock, 10k resistors (+- 1%), BSS138-7-F MOSFETs, XIAO_ESP32C3

    TIP_PIN  = D1  (GPIO3)
    RING_PIN = D10 (GPIO10)
    Sleeve   = GND

  Arduino IDE settings:
    Board:           XIAO_ESP32C3
    USB CDC On Boot: Enabled
    Serial Monitor:  115200 baud

  Usage in the Serial Monitor (type a letter + Enter):
    a = output test  (ESP32 -> jack, measure at the jack with a multimeter)
    e = input test   (jack -> ESP32, short Tip/Ring to GND at the jack)
    t = hold TIP low only
    r = hold RING low only
    l = release both lines (idle state, both HIGH)
    s = show current line state once
    h = help

  IMPORTANT: Test WITHOUT a calculator connected!
  The pins are only used as open-drain outputs (pull LOW or release),
  they are never actively driven HIGH.
*/

const int TIP_PIN  = D1;
const int RING_PIN = D10;

// ---------- Helper functions ----------

// Pull the line LOW
void pullLow(int pin) {
  digitalWrite(pin, LOW);
}

// Release the line -> the pull-up makes it HIGH
void release(int pin) {
  digitalWrite(pin, HIGH);
}

const char* levelName(int pin) {
  return digitalRead(pin) ? "HIGH" : "LOW ";
}

void printState() {
  Serial.print("  TIP = ");
  Serial.print(levelName(TIP_PIN));
  Serial.print("   RING = ");
  Serial.println(levelName(RING_PIN));
}

// Waits for the given time, but returns early (true) as soon as
// something is typed in the Serial Monitor
bool waitOrAbort(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (Serial.available()) return true;
    delay(10);
  }
  return false;
}

void printHelp() {
  Serial.println();
  Serial.println("=== TI-Link PCB test ===");
  Serial.println("a = output test (ESP32 -> jack)");
  Serial.println("e = input test (jack -> ESP32)");
  Serial.println("t = hold TIP low only");
  Serial.println("r = hold RING low only");
  Serial.println("l = release both lines (idle state)");
  Serial.println("s = show line state");
  Serial.println("h = show this help");
  Serial.println("During a test: any key + Enter = abort");
  Serial.println();
}

// ---------- Tests ----------

void outputTest() {
  Serial.println();
  Serial.println("--- Output test: ESP32 -> jack ---");
  Serial.println("Connect the multimeter between Sleeve (GND) and Tip or Ring at the jack.");
  Serial.println("Expected: HIGH = pull-up voltage (e.g. 5 V), LOW = approx. 0 V.");
  Serial.println("Tip and Ring alternate in OPPOSITE phase (never both the same).");
  Serial.println("Press any key to abort.");

  while (true) {
    pullLow(TIP_PIN);
    release(RING_PIN);
    Serial.println("Expected:  TIP = LOW,  RING = HIGH");
    printState();
    if (waitOrAbort(3000)) break;

    release(TIP_PIN);
    pullLow(RING_PIN);
    Serial.println("Expected:  TIP = HIGH, RING = LOW");
    printState();
    if (waitOrAbort(3000)) break;
  }

  release(TIP_PIN);
  release(RING_PIN);
  Serial.println("Output test finished, both lines released.");
}

void inputTest() {
  Serial.println();
  Serial.println("--- Input test: jack -> ESP32 ---");
  Serial.println("Idle state: both lines HIGH.");
  Serial.println("Briefly connect Tip or Ring to Sleeve (GND) at the jack.");
  Serial.println("Expected: only the connected line goes LOW.");
  Serial.println("Press any key to abort.");

  release(TIP_PIN);
  release(RING_PIN);

  int lastTip = -1, lastRing = -1;
  while (!Serial.available()) {
    int tip  = digitalRead(TIP_PIN);
    int ring = digitalRead(RING_PIN);
    // Only print on change, so transitions are easy to spot
    if (tip != lastTip || ring != lastRing) {
      printState();
      lastTip = tip;
      lastRing = ring;
    }
    delay(20);
  }
  Serial.println("Input test finished.");
}

// ---------- Setup / Loop ----------

void setup() {
  Serial.begin(115200);

  // Open-drain: LOW pulls the line down, HIGH only releases it
  pinMode(TIP_PIN, OUTPUT_OPEN_DRAIN);
  pinMode(RING_PIN, OUTPUT_OPEN_DRAIN);
  release(TIP_PIN);
  release(RING_PIN);

  // Wait briefly so the Serial Monitor can connect
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000) {
    delay(10);
  }

  printHelp();
  printState();
}

void loop() {
  if (!Serial.available()) return;

  char c = Serial.read();
  // Ignore line endings and spaces
  if (c == '\n' || c == '\r' || c == ' ') return;
  // Discard the rest of the line
  delay(5);
  while (Serial.available()) Serial.read();

  switch (c) {
    case 'a': case 'A': outputTest(); break;
    case 'e': case 'E': inputTest(); break;
    case 't': case 'T':
      pullLow(TIP_PIN); release(RING_PIN);
      Serial.println("TIP held LOW, RING released.");
      printState();
      break;
    case 'r': case 'R':
      release(TIP_PIN); pullLow(RING_PIN);
      Serial.println("RING held LOW, TIP released.");
      printState();
      break;
    case 'l': case 'L':
      release(TIP_PIN); release(RING_PIN);
      Serial.println("Both lines released.");
      printState();
      break;
    case 's': case 'S': printState(); break;
    case 'h': case 'H': printHelp(); break;
    default:
      Serial.print("Unknown command: ");
      Serial.println(c);
      printHelp();
  }

  // Discard any input that was typed to abort a test
  delay(5);
  while (Serial.available()) Serial.read();
}
