import asyncio
import json
import pathlib
from typing import AsyncIterator

LOG_PATH = pathlib.Path(__file__).parent.parent.parent / "shared" / "battle_log.jsonl"


class FileWatcher:
    """Async tail of battle_log.jsonl — yields parsed JSON dicts as lines arrive."""

    def __init__(self, path: pathlib.Path = LOG_PATH, poll_interval: float = 0.05):
        self._path     = path
        self._interval = poll_interval
        self._offset   = 0
        self._file     = None

    def reset(self) -> None:
        if self._file:
            self._file.close()
            self._file = None
        self._offset = 0

    async def tail(self) -> AsyncIterator[dict]:
        while True:
            if not self._path.exists():
                await asyncio.sleep(self._interval)
                continue
            if self._file is None:
                self._file = open(self._path, "r")
            self._file.seek(self._offset)
            while True:
                line = self._file.readline()
                if not line:
                    break
                self._offset = self._file.tell()
                line = line.strip()
                if not line:
                    continue
                try:
                    yield json.loads(line)
                except json.JSONDecodeError:
                    continue
            await asyncio.sleep(self._interval)

    def close(self) -> None:
        if self._file:
            self._file.close()
            self._file = None
