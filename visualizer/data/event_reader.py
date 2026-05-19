import json
import pathlib
from dataclasses import dataclass
from typing import Dict, List, Optional


@dataclass
class TankState:
    entity_id: int
    team: str        # 'A' or 'B'
    hp: int
    max_hp: int
    alive: bool = True


class EventReader:
    def __init__(self, log_path: pathlib.Path,
                 tanks_per_team: int = 64,
                 first_team_b_id: int = 64):
        self._path          = log_path
        self._tanks_per_team = tanks_per_team
        self._first_b       = first_team_b_id
        self._file          = None
        self._offset        = 0

        self.tanks:   Dict[int, TankState] = {}
        self.events:  List[str]            = []   # recent human-readable strings
        self.tick:    int                  = 0
        self.kills_a: int                  = 0
        self.kills_b: int                  = 0
        self._init_tanks()

    # ── Public API ────────────────────────────────────────────────────────────

    def restart(self) -> None:
        if self._file:
            self._file.close()
            self._file = None
        self._offset = 0
        self.events.clear()
        self.tick    = 0
        self.kills_a = 0
        self.kills_b = 0
        self._init_tanks()

    def poll(self) -> int:
        """Non-blocking read of any new log lines. Returns count of new events."""
        if not self._path.exists():
            return 0
        if self._file is None:
            self._file = open(self._path, "r")
        self._file.seek(self._offset)
        count = 0
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
            self._apply(ev)
            count += 1
        return count

    def close(self) -> None:
        if self._file:
            self._file.close()
            self._file = None

    @property
    def alive_a(self) -> int:
        return sum(1 for t in self.tanks.values() if t.team == "A" and t.alive)

    @property
    def alive_b(self) -> int:
        return sum(1 for t in self.tanks.values() if t.team == "B" and t.alive)

    @property
    def winner(self) -> Optional[str]:
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

    # ── Internal ──────────────────────────────────────────────────────────────

    def _init_tanks(self) -> None:
        self.tanks.clear()
        total = self._tanks_per_team * 2
        for i in range(total):
            team = "A" if i < self._first_b else "B"
            self.tanks[i] = TankState(
                entity_id=i, team=team, hp=100, max_hp=100
            )

    def _apply(self, ev: dict) -> None:
        t = ev.get("type")

        if t == "damage_dealt":
            target_id = ev.get("target")
            dmg       = ev.get("damage", 0)
            shooter   = ev.get("entity")
            if target_id in self.tanks:
                tank = self.tanks[target_id]
                tank.hp = max(0, tank.hp - dmg)
            self.events.append(f"E{shooter:>3} → E{target_id:<3}  -{dmg} hp")
            self.tick += 1

        elif t == "tank_destroyed":
            entity_id = ev.get("entity")
            killer    = ev.get("killed_by")
            if entity_id in self.tanks:
                tank = self.tanks[entity_id]
                tank.alive = False
                tank.hp    = 0
                if tank.team == "A":
                    self.kills_b += 1
                else:
                    self.kills_a += 1
            self.events.append(f"E{entity_id:<3} destroyed by E{killer}")

        # Keep internal buffer bounded
        if len(self.events) > 500:
            self.events = self.events[-500:]
