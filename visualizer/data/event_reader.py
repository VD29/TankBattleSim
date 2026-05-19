import json
import pathlib
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple


@dataclass
class TankState:
    entity_id: int
    team: str            # 'A' or 'B'
    hp: int
    max_hp: int
    alive: bool = True
    x: float = 0.0
    y: float = 0.0
    has_position: bool = False


class EventReader:
    """
    Buffered replay reader.

    _load_file() pulls any new log lines into _buffer, tagged with the
    display tick of the most-recent tank_moved event seen so far (combat
    events inherit the same tick).

    advance() / poll() walk _play_idx forward through _buffer up to
    self.tick, calling _apply() on each event.  The caller controls
    how fast self.tick grows; at 1 step/frame @ 60 FPS the replay runs
    at ~60 sim-ticks per second.
    """

    def __init__(self, log_path: pathlib.Path,
                 tanks_per_team: int = 64,
                 first_team_b_id: int = 64):
        self._path           = log_path
        self._tanks_per_team = tanks_per_team
        self._first_b        = first_team_b_id
        self._file           = None
        self._offset: int    = 0

        # [(display_tick, event_dict)] — grown by _load_file(), consumed by _drain()
        self._buffer:       List[Tuple[int, dict]] = []
        self._buf_log_tick: int = 0   # tick tag inherited by non-positional events
        self._play_idx:     int = 0   # next index to apply

        self.tanks:   Dict[int, TankState] = {}
        self.events:  List[str]            = []
        self.tick:    int                  = 0
        self.kills_a: int                  = 0
        self.kills_b: int                  = 0
        self._init_tanks()

    # ── Public API ────────────────────────────────────────────────────────────

    def advance(self, steps: int = 1) -> None:
        """Advance replay by `steps` sim ticks and apply newly-due events."""
        self._load_file()
        self.tick += steps
        self._drain()

    def poll(self) -> None:
        """Load new file content and apply events up to current tick (paused)."""
        self._load_file()
        self._drain()

    def restart(self) -> None:
        """Rewind to tick 0; keep the buffer so the file is not re-read."""
        self._play_idx = 0
        self.tick      = 0
        self.kills_a   = 0
        self.kills_b   = 0
        self.events.clear()
        self._init_tanks()

    def close(self) -> None:
        if self._file:
            self._file.close()
            self._file = None

    # ── Properties ────────────────────────────────────────────────────────────

    @property
    def alive_a(self) -> int:
        return sum(1 for t in self.tanks.values() if t.team == "A" and t.alive)

    @property
    def alive_b(self) -> int:
        return sum(1 for t in self.tanks.values() if t.team == "B" and t.alive)

    @property
    def winner(self) -> Optional[str]:
        # Only declare a winner once the replay has caught up to all events.
        if self._play_idx < len(self._buffer):
            return None
        a, b = self.alive_a, self.alive_b
        if a == 0 and b == 0:
            return "Draw"
        if a == 0:
            return "Team B"
        if b == 0:
            return "Team A"
        return None

    @property
    def recent_events(self) -> List[str]:
        return self.events[-10:]

    # ── Internals ─────────────────────────────────────────────────────────────

    def _init_tanks(self) -> None:
        self.tanks.clear()
        for i in range(self._tanks_per_team * 2):
            team = "A" if i < self._first_b else "B"
            self.tanks[i] = TankState(entity_id=i, team=team, hp=100, max_hp=100)

    def _load_file(self) -> None:
        """Non-blocking: append any new log lines to _buffer."""
        if not self._path.exists():
            return
        if self._file is None:
            self._file = open(self._path, "r")
        self._file.seek(self._offset)
        while True:
            raw = self._file.readline()
            if not raw:
                break
            self._offset = self._file.tell()
            raw = raw.strip()
            if not raw:
                continue
            try:
                ev = json.loads(raw)
            except json.JSONDecodeError:
                continue
            if ev.get("type") == "tank_moved":
                self._buf_log_tick = int(ev.get("tick", self._buf_log_tick))
            self._buffer.append((self._buf_log_tick, ev))

    def _drain(self) -> None:
        """Apply buffered events whose display_tick <= self.tick."""
        while self._play_idx < len(self._buffer):
            display_tick, ev = self._buffer[self._play_idx]
            if display_tick > self.tick:
                break
            self._apply(ev)
            self._play_idx += 1

    def _apply(self, ev: dict) -> None:
        t = ev.get("type")

        if t == "tank_moved":
            eid = ev.get("id") if "id" in ev else ev.get("entity")
            if eid in self.tanks:
                tank = self.tanks[eid]
                tank.x = float(ev.get("x", tank.x))
                tank.y = float(ev.get("y", tank.y))
                tank.has_position = True
            return   # silent — no entry in event log

        if t == "damage_dealt":
            target  = ev.get("target")
            dmg     = ev.get("damage", 0)
            shooter = ev.get("entity")
            if target in self.tanks:
                self.tanks[target].hp = max(0, self.tanks[target].hp - dmg)
            self.events.append(f"E{shooter:>3} → E{target:<3}  -{dmg} hp")

        elif t == "tank_destroyed":
            eid    = ev.get("entity")
            killer = ev.get("killed_by")
            if eid in self.tanks:
                tank = self.tanks[eid]
                tank.alive = False
                tank.hp    = 0
                if tank.team == "A":
                    self.kills_b += 1
                else:
                    self.kills_a += 1
            self.events.append(f"E{eid:<3} destroyed by E{killer}")

        if len(self.events) > 500:
            self.events = self.events[-500:]
