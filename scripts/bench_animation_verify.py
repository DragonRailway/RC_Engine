#!/usr/bin/env python3
"""MIKRO_V2 bench verification for the EasyKit animation engine.

Drives RadioKit widgets over serial while the firmware prints a `[bench]`
line every ~100 ms (temporary debug hook in HardwareInit::debugAnimationReport):

  [bench] aux1=<us> aux2=<us> head=<pct> tail=<pct> turnL=<pct> turnR=<pct> brake=<pct>

Verifies the three animation behaviors from task 5.3:
  1. Turn-signal/hazard blink at the config interval (500/500 ms -> 1 Hz)
  2. Headlight 3-state stepping fades over fade_duration_ms (250 ms) with the
     tail tracking the headlight's live duty
  3. Aux-servo easing (sweep moves gradually through intermediate positions,
     not instant)

Usage: python3 scripts/bench_animation_verify.py [PORT]
"""
import serial, time, re, sys

PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM0'
BAUD = 2000000

# Widget IDs (declaration order in src/RADIOKIT.h)
W_STEER, W_GAS, W_BRAKE, W_TRUCK_LIGHT, W_START, W_LEFT_IND, W_RIGHT_IND, \
W_AUX, W_HORN, W_GEAR, W_THROTTLE, W_DIR, W_LOCO_LIGHT, W_BELL, W_ENGINE = range(15)

BENCH_RE = re.compile(r'\[bench\] aux1=(-?\d+)us aux2=(-?\d+)us head=(\d+)% tail=(\d+)% turnL=(\d+)% turnR=(\d+)% brake=(\d+)%')

def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= (b << 8) & 0xFFFF
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def frame(cmd, payload=b''):
    body = bytes([cmd]) + payload
    crc = crc16(body)
    return bytes([0x55, (4 + len(payload) + 2) & 0xFF, ((4 + len(payload) + 2) >> 8) & 0xFF, cmd]) + payload + bytes([crc & 0xFF, (crc >> 8) & 0xFF])

VAR_UPDATE = 0x08

class Bench:
    def __init__(self, ser):
        self.ser = ser
        self.seq = 0
        self.samples = []   # (t, dict)

    def send(self, cmd, payload=b''):
        self.seq = (self.seq + 1) & 0xFF
        self.ser.write(frame(cmd, payload))
        self.ser.flush()

    def u(self, wid, value):
        self.send(VAR_UPDATE, bytes([wid, self.seq, value & 0xFF]))

    def capture(self, seconds, label):
        """Collect [bench] samples for `seconds`, tagged with wall-clock time."""
        end = time.monotonic() + seconds
        got = []
        while time.monotonic() < end:
            c = self.ser.read(8192)
            if c:
                try:
                    txt = c.decode('utf-8', errors='replace')
                except Exception:
                    continue
                for line in txt.splitlines():
                    m = BENCH_RE.search(line)
                    if m:
                        got.append((time.monotonic(), {
                            'aux1': int(m.group(1)), 'aux2': int(m.group(2)),
                            'head': int(m.group(3)), 'tail': int(m.group(4)),
                            'turnL': int(m.group(5)), 'turnR': int(m.group(6)),
                            'brake': int(m.group(7)),
                        }))
            else:
                time.sleep(0.02)
        if not got:
            print(f"  [WARN] no [bench] samples captured during '{label}'")
        self.samples.extend((t, dict(s)) for t, s in got)
        return got

    def drain(self, seconds=0.3):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            if self.ser.read(4096):
                time.sleep(0.01)
            else:
                time.sleep(0.02)

def analyze_blink(samples):
    """Expect turnL to alternate 60/0 with ~500 ms on/off periods (1 Hz cycle)."""
    if not samples:
        return False, "no samples"
    duties = [s['turnL'] for _, s in samples]
    highs = sum(1 for d in duties if d >= 50)
    lows = sum(1 for d in duties if d <= 5)
    on_off_both_seen = highs > 0 and lows > 0
    # measure on/off durations from transitions
    transitions = []
    for i in range(1, len(duties)):
        if (duties[i] >= 50) != (duties[i-1] >= 50):
            transitions.append((samples[i][0] - samples[0][0]))
    if len(transitions) < 3:
        return False, f"only {len(transitions)} transitions (duties={duties})"
    durations = [transitions[i+1] - transitions[i] for i in range(len(transitions)-1)]
    avg = sum(durations) / len(durations)
    ok = on_off_both_seen and 0.30 <= avg <= 0.75
    return ok, f"duty seq={duties} | on/off avg={avg:.2f}s (expect ~0.5s), {len(transitions)} transitions"

def analyze_fade(samples, target, start=0):
    """Head duty should ramp start -> target through intermediate values (not a jump)."""
    if not samples:
        return False, "no samples"
    duties = [s['head'] for _, s in samples]
    reached = any(d >= target * 0.95 for d in duties)
    # count distinct intermediate values strictly between the start level and target
    inter = set(d for d in duties if start < d < target * 0.95)
    ok = reached and len(inter) >= 2
    return ok, f"head seq={duties} | intermediates={sorted(inter)} (expect ramp {start}->{target})"

def analyze_tail_tracks(samples):
    """tail should be ~0.3 * head live duty (following the fade)."""
    if not samples:
        return False, "no samples"
    ok = True
    for _, s in samples:
        expect = round(s['head'] * 0.3)
        if abs(s['tail'] - expect) > 4:
            ok = False
    return ok, f"tail tracked head (0.3x) within 4%%"

def analyze_easing(samples):
    """aux1 should move 1500 -> 2000 us gradually over ~1s (not instant)."""
    if not samples:
        return False, "no samples"
    us = [s['aux1'] for _, s in samples]
    reached = us[-1] >= 1990 if us else False
    inter = set(u for u in us if 1520 < u < 1980)
    first = samples[0][0]
    last = samples[-1][0]
    span = last - first
    ok = reached and len(inter) >= 4 and span >= 0.4
    return ok, f"aux1 us={us} | {len(inter)} intermediates, span={span:.2f}s (expect ~1s eased sweep)"

def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.3)
    print(f"Opened {PORT} @ {BAUD}", flush=True)
    ser.setDTR(False); ser.setRTS(True); time.sleep(0.1)
    ser.setDTR(True); ser.setRTS(False); time.sleep(0.05)
    ser.reset_input_buffer()

    # Wait for clean boot
    print("waiting for clean boot...", flush=True)
    end = time.time() + 12
    booted = False
    while time.time() < end:
        c = ser.read(4096)
        if c and b'System Ready' in c:
            booted = True
            break
        time.sleep(0.05)
    if not booted:
        print("  [FAIL] no clean boot observed", flush=True)
        ser.close()
        sys.exit(1)
    ser.reset_input_buffer()

    b = Bench(ser)
    results = []

    # ── Phase 1: turn-signal blink at config interval ──
    print("\n── PHASE 1: left indicator blink (expect 500/500 ms = 1 Hz) ──", flush=True)
    b.drain(0.5)
    b.u(W_LEFT_IND, 1)
    samp = b.capture(2.6, "blink")
    b.u(W_LEFT_IND, 0)
    ok, msg = analyze_blink(samp)
    results.append(("turn blink 1 Hz", ok, msg))
    print(f"  [{'PASS' if ok else 'FAIL'}] {msg}", flush=True)

    # ── Phase 2: headlight fade to low beam (40% of 60 = 24) ──
    # Capture DURING the fade: the rising edge triggers setLightFade and the
    # ramp completes in 250ms, so release only after the capture window.
    print("\n── PHASE 2: headlight step -> low beam fade (expect 0->24 over 250ms) ──", flush=True)
    b.drain(0.5)
    b.u(W_TRUCK_LIGHT, 0x00)          # ensure released
    b.drain(0.3)
    b.u(W_TRUCK_LIGHT, 0x01)          # rising edge: mode 0 -> 1 (low beam) -> fade starts
    samp = b.capture(0.9, "head fade low")
    b.u(W_TRUCK_LIGHT, 0x00)          # release after capture
    ok, msg = analyze_fade(samp, 24)
    results.append(("headlight fade low", ok, msg))
    print(f"  [{'PASS' if ok else 'FAIL'}] {msg}", flush=True)
    ok2, msg2 = analyze_tail_tracks(samp)
    results.append(("tail tracks head (low)", ok2, msg2))
    print(f"  [{'PASS' if ok2 else 'FAIL'}] {msg2}", flush=True)

    # ── Phase 3: headlight fade to high beam (100% of 60 = 60) ──
    print("\n── PHASE 3: headlight step -> high beam fade (expect 24->60 over 250ms) ──", flush=True)
    b.drain(0.5)
    b.u(W_TRUCK_LIGHT, 0x01)          # rising edge: mode 1 -> 2 (high beam) -> fade starts
    samp = b.capture(0.9, "head fade high")
    b.u(W_TRUCK_LIGHT, 0x00)          # release after capture
    ok, msg = analyze_fade(samp, 60, start=24)
    results.append(("headlight fade high", ok, msg))
    print(f"  [{'PASS' if ok else 'FAIL'}] {msg}", flush=True)
    ok2, msg2 = analyze_tail_tracks(samp)
    results.append(("tail tracks head (high)", ok2, msg2))
    print(f"  [{'PASS' if ok2 else 'FAIL'}] {msg2}", flush=True)
    b.u(W_TRUCK_LIGHT, 0x00)

    # ── Phase 4: aux-servo easing sweep 0 -> 100 ──
    print("\n── PHASE 4: aux servo eased sweep (expect 1500->2000us over ~1s) ──", flush=True)
    b.drain(0.5)
    b.u(W_AUX, 0)
    b.drain(0.5)
    b.u(W_AUX, 100)
    samp = b.capture(1.8, "aux sweep")
    b.u(W_AUX, 0)
    b.drain(1.0)
    ok, msg = analyze_easing(samp)
    results.append(("aux servo easing", ok, msg))
    print(f"  [{'PASS' if ok else 'FAIL'}] {msg}", flush=True)

    # ── Summary ──
    print("\n══ BENCH ASSESSMENT ══", flush=True)
    allok = True
    for name, ok, msg in results:
        allok = allok and ok
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}: {msg}", flush=True)
    print(f"\n{'ALL BENCH ANIMATION CHECKS PASSED' if allok else 'SOME BENCH CHECKS FAILED'}", flush=True)
    ser.close()
    sys.exit(0 if allok else 1)

if __name__ == '__main__':
    main()
