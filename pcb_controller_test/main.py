"""
TI-84 -> PC Maus + Tastatur
Liest Zeilen wie "KEY:25" vom ESP32 (Serial).
Pfeiltasten bewegen die Maus, ENTER = Linksklick, 2nd = Rechtsklick,
ALPHA schaltet zwischen Ziffern- und Buchstabenmodus um,
alle anderen belegten Tasten werden als Tastendruck weitergegeben.
"""

import time
import serial
import pyautogui

# --- Einstellungen ---------------------------------------------------------
PORT = "COM4"   # im Geräte-Manager unter "Anschlüsse (COM & LPT)" nachsehen
BAUD = 9600     # muss zu Serial.begin(...) im Arduino-Sketch passen

SCHRITT_START = 15     # Pixel pro Tastendruck am Anfang
SCHRITT_MAX   = 120    # maximale Pixel pro Tastendruck (beim Gedrückthalten)
BESCHLEUNIGUNG = 1.4   # Faktor pro Wiederholung in dieselbe Richtung
WIEDERHOL_ZEIT = 0.3   # Sekunden: kommt die nächste gleiche Taste schneller, wird beschleunigt

pyautogui.PAUSE = 0    # sonst wartet pyautogui nach jedem Befehl 0,1 s

# Pfeiltasten -> Bewegungsrichtung (dx, dy)
MAUS = {
    24: (-1, 0),   # links
    26: (1, 0),    # rechts
    25: (0, -1),   # hoch
    34: (0, 1),    # runter
}

# Maustasten
KLICKS = {
    105: "left",   # ENTER
    21: "right",   # 2nd
}

# übrige Tasten -> Tastendruck (Code = Zeile * 10 + Spalte auf dem TI-84)
TASTEN = {
    # Pfeiltasten der Tastatur (die echten TI-Pfeile steuern die Maus)
    22: "up",       # MODE
    32: "left",     # X,T,θ,n
    33: "right",    # STAT
    44: "down",     # VARS
    # Sondertasten
    45: "esc", 23: "backspace",
    11: "f1", 12: "f2", 13: "f3", 14: "f4", 15: "f5",
    102: "0", 92: "1", 93: "2", 94: "3",
    82: "4", 83: "5", 84: "6",
    72: "7", 73: "8", 74: "9",
    103: ".",
    95: "+", 85: "-", 75: "*", 65: "/",
    63: "(", 64: ")", 62: ",",
}

ALPHA_TASTE = 31       # ALPHA-Taste auf dem TI-84
GROSSBUCHSTABEN = False  # True: A-Z statt a-z

# Buchstabenmodus: Belegung wie die grüne Beschriftung auf dem TI-84
BUCHSTABEN = {
    41: "a", 42: "b", 43: "c",                       # MATH APPS PRGM
    51: "d", 52: "e", 53: "f", 54: "g", 55: "h",     # x⁻¹ SIN COS TAN ^
    61: "i", 62: "j", 63: "k", 64: "l", 65: "m",     # x² , ( ) ÷
    71: "n", 72: "o", 73: "p", 74: "q", 75: "r",     # LOG 7 8 9 ×
    81: "s", 82: "t", 83: "u", 84: "v", 85: "w",     # LN 4 5 6 −
    91: "x", 92: "y", 93: "z",                       # STO→ 1 2
    102: "space", 103: ":", 104: "?",                # 0 . (−)
}
# ---------------------------------------------------------------------------


def main():
    letzte_taste = None
    letzte_zeit = 0.0
    schritt = SCHRITT_START
    alpha_modus = False

    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        print(f"Verbunden mit {PORT} ({BAUD} Baud). Beenden mit Strg+C.")

        while True:
            zeile = ser.readline().decode(errors="ignore").strip()
            if not zeile:
                continue

            if not zeile.startswith("KEY:"):
                print(f"[ESP32] {zeile}")
                continue

            try:
                code = int(zeile[4:])
            except ValueError:
                print(f"Ungültige Zeile: {zeile}")
                continue

            jetzt = time.monotonic()

            if code in MAUS:
                # gleiche Richtung kurz hintereinander -> schneller werden
                if code == letzte_taste and jetzt - letzte_zeit < WIEDERHOL_ZEIT:
                    schritt = min(schritt * BESCHLEUNIGUNG, SCHRITT_MAX)
                else:
                    schritt = SCHRITT_START

                dx, dy = MAUS[code]
                pyautogui.moveRel(dx * schritt, dy * schritt)

            elif code == ALPHA_TASTE:
                alpha_modus = not alpha_modus
                print(f"--- Modus: {'BUCHSTABEN' if alpha_modus else 'ZIFFERN'} ---")

            elif alpha_modus and code in BUCHSTABEN:
                zeichen = BUCHSTABEN[code]
                if GROSSBUCHSTABEN and len(zeichen) == 1 and zeichen.isalpha():
                    pyautogui.hotkey("shift", zeichen)
                else:
                    pyautogui.press(zeichen)
                print(f"TI {code:>3} -> {zeichen}")

            elif code in KLICKS:
                pyautogui.click(button=KLICKS[code])
                print(f"TI {code:>3} -> Klick {KLICKS[code]}")

            elif code in TASTEN:
                pyautogui.press(TASTEN[code])
                print(f"TI {code:>3} -> {TASTEN[code]}")

            else:
                print(f"TI {code:>3} -> (nicht belegt)")

            letzte_taste = code
            letzte_zeit = jetzt


if __name__ == "__main__":
    try:
        main()
    except serial.SerialException as e:
        print(f"Serial-Fehler: {e}")
        print("Richtiger COM-Port? Serial Monitor der Arduino IDE geschlossen?")
    except pyautogui.FailSafeException:
        print("\nNotbremse: Maus in der Bildschirmecke. Beendet.")
    except KeyboardInterrupt:
        print("\nBeendet.")