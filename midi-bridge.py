#!/usr/bin/env python3
import sys, time, argparse
import serial
from serial.tools import list_ports
import rtmidi

def list_serial_ports():
    ports = list_ports.comports()
    if not ports:
        print("No serial ports found.")
    else:
        print("Available serial ports:")
        for p in ports:
            print(f"  {p.device}  -  {p.description}")

def open_serial(port, baud, timeout=1.0):
    try:
        ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)
        # Give the CDC device a moment after opening
        time.sleep(0.2)
        return ser
    except serial.SerialException as e:
        print(f"ERROR opening serial port {port}: {e}\n")
        list_serial_ports()
        sys.exit(1)

def open_virtual_midi(port_name):
    midi_out = rtmidi.MidiOut()
    midi_out.open_virtual_port(port_name)
    print(f"Opened virtual MIDI OUT: {port_name}")
    return midi_out

def main():
    ap = argparse.ArgumentParser(description="Serial → Virtual MIDI bridge")
    ap.add_argument("--serial", "-s", required=False,
                    help="Serial port (e.g. /dev/cu.usbmodem101). If omitted, list and exit.")
    ap.add_argument("--baud", "-b", type=int, default=115200,
                    help="Serial baud (default: 115200)")
    ap.add_argument("--name", "-n", default="UNO-R4-Serial-MIDI",
                    help="Virtual MIDI port name (default: UNO-R4-Serial-MIDI)")
    args = ap.parse_args()

    if not args.serial:
        list_serial_ports()
        print("\nPass --serial PORT to start bridging.")
        sys.exit(0)

    ser = open_serial(args.serial, args.baud)
    midi_out = open_virtual_midi(args.name)

    print(f"Bridging {args.serial}@{args.baud} → virtual MIDI '{args.name}'")
    print("Press Ctrl+C to quit.")

    # This bridge assumes each message is a 3-byte MIDI msg (status, data1, data2)
    # like your Arduino NoteOn/NoteOff. If you later send other message types,
    # extend the parser accordingly.
    try:
        while True:
            # Block until we get 3 bytes; if your stream is bursty, this is fine.
            data = ser.read(3)
            if len(data) != 3:
                continue  # read timed out; loop again

            status = data[0] & 0xF0
            # Basic sanity: only forward valid 3-byte Channel Voice messages
            if status in (0x80, 0x90, 0xA0, 0xB0, 0xE0):  # NoteOff, NoteOn, PolyAT, CC, PitchBend
                midi_out.send_message([data[0], data[1] & 0x7F, data[2] & 0x7F])
            elif status == 0xC0 or status == 0xD0:
                # Program Change / Channel Aftertouch are 2-byte messages.
                # If you ever send these, adjust Arduino to pad with a dummy third byte
                # or update this script to read only 2 bytes for these statuses.
                pass
            else:
                # Ignore anything else (e.g., stray text, line breaks)
                pass
    except KeyboardInterrupt:
        print("\nExiting…")
    finally:
        try:
            midi_out.close_port()
        except Exception:
            pass
        ser.close()

if __name__ == "__main__":
    main()
