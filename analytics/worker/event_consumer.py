import asyncio
import pathlib
from typing import Optional

from analytics.db.crud import (
    create_battle, finish_battle, record_event, update_tank_stats,
)
from analytics.db.database import AsyncSessionLocal, init_db
from analytics.worker.file_watcher import FileWatcher

LOG_PATH   = pathlib.Path(__file__).parent.parent.parent / "shared" / "battle_log.jsonl"
BATCH_SIZE = 50


class EventConsumer:
    def __init__(self) -> None:
        self._watcher  = FileWatcher(LOG_PATH)
        self._running  = False
        self._task: Optional[asyncio.Task] = None

    async def start(self) -> None:
        await init_db()
        self._running = True
        self._task = asyncio.create_task(self._run())

    async def stop(self) -> None:
        self._running = False
        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass
        self._watcher.close()

    async def _run(self) -> None:
        async with AsyncSessionLocal() as db:
            battle    = await create_battle(db)
            battle_id = battle.id
            batch:    list[dict] = []
            total_ev  = 0
            last_tick = 0

            async for event in self._watcher.tail():
                if not self._running:
                    break

                ev_type = event.get("type")

                # Position events: track tick but skip DB insert (too voluminous)
                if ev_type == "tank_moved":
                    last_tick = max(last_tick, int(event.get("tick", 0)))
                    continue

                batch.append(event)
                total_ev += 1

                if len(batch) >= BATCH_SIZE:
                    for ev in batch:
                        await record_event(db, battle_id, ev)
                        await update_tank_stats(db, battle_id, ev)
                    await db.commit()
                    batch.clear()

            # Flush remaining events
            for ev in batch:
                await record_event(db, battle_id, ev)
                await update_tank_stats(db, battle_id, ev)
            if batch:
                await db.commit()

            await finish_battle(db, battle_id,
                                winner="unknown",
                                total_ticks=last_tick,
                                total_events=total_ev)
