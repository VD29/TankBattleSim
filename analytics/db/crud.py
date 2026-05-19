from datetime import datetime, timezone
from typing import Optional

from sqlalchemy import func, select, update
from sqlalchemy.ext.asyncio import AsyncSession

from analytics.models.battle import Battle, BattleEvent
from analytics.models.tank import TankStats

_FIRST_B_ID = 64   # entity IDs 0-63 = Team A, 64-127 = Team B


def _team(entity_id: int) -> str:
    return "A" if entity_id < _FIRST_B_ID else "B"


# ── Battle ────────────────────────────────────────────────────────────────────

async def create_battle(db: AsyncSession) -> Battle:
    battle = Battle()
    db.add(battle)
    await db.commit()
    await db.refresh(battle)
    return battle


async def finish_battle(db: AsyncSession, battle_id: int,
                        winner: str, total_ticks: int, total_events: int) -> None:
    await db.execute(
        update(Battle)
        .where(Battle.id == battle_id)
        .values(
            ended_at=datetime.now(timezone.utc),
            winner_team=winner,
            total_ticks=total_ticks,
            total_events=total_events,
        )
    )
    await db.commit()


async def get_battle_stats(db: AsyncSession, battle_id: int) -> Optional[dict]:
    result = await db.execute(select(Battle).where(Battle.id == battle_id))
    b = result.scalar_one_or_none()
    if b is None:
        return None
    return {
        "id":           b.id,
        "started_at":   b.started_at,
        "ended_at":     b.ended_at,
        "winner_team":  b.winner_team,
        "total_ticks":  b.total_ticks,
        "total_events": b.total_events,
    }


# ── Events ────────────────────────────────────────────────────────────────────

async def record_event(db: AsyncSession, battle_id: int, event: dict) -> None:
    db.add(BattleEvent(
        battle_id=battle_id,
        event_type=event.get("type", "unknown"),
        entity_id=event.get("entity"),
        target_id=event.get("target"),
        damage=event.get("damage"),
        x=event.get("x"),
        y=event.get("y"),
        tick=event.get("tick"),
    ))


# ── Tank stats ────────────────────────────────────────────────────────────────

async def update_tank_stats(db: AsyncSession, battle_id: int, event: dict) -> None:
    t = event.get("type")
    if t == "damage_dealt":
        shooter = event.get("entity")
        target  = event.get("target")
        dmg     = int(event.get("damage", 0))
        if shooter is not None:
            await _inc(db, battle_id, shooter, damage_dealt=dmg, shots_fired=1, shots_hit=1)
        if target is not None:
            await _inc(db, battle_id, target, damage_taken=dmg)
    elif t == "tank_destroyed":
        eid    = event.get("entity")
        killer = event.get("killed_by")
        if eid is not None:
            await _inc(db, battle_id, eid, deaths=1)
        if killer is not None:
            await _inc(db, battle_id, killer, kills=1)


async def _inc(db: AsyncSession, battle_id: int, tank_id: int,
               kills: int = 0, deaths: int = 0,
               damage_dealt: int = 0, damage_taken: int = 0,
               shots_fired: int = 0, shots_hit: int = 0) -> None:
    result = await db.execute(
        select(TankStats).where(
            TankStats.tank_id  == tank_id,
            TankStats.battle_id == battle_id,
        )
    )
    ts = result.scalar_one_or_none()
    if ts is None:
        ts = TankStats(tank_id=tank_id, battle_id=battle_id, team=_team(tank_id))
        db.add(ts)
    ts.kills        += kills
    ts.deaths       += deaths
    ts.damage_dealt += damage_dealt
    ts.damage_taken += damage_taken
    ts.shots_fired  += shots_fired
    ts.shots_hit    += shots_hit


async def get_tank_performance(db: AsyncSession, tank_id: int) -> list:
    result = await db.execute(
        select(TankStats)
        .where(TankStats.tank_id == tank_id)
        .order_by(TankStats.battle_id.desc())
    )
    rows = result.scalars().all()
    return [
        {
            "tank_id":      r.tank_id,
            "battle_id":    r.battle_id,
            "team":         r.team,
            "kills":        r.kills,
            "deaths":       r.deaths,
            "damage_dealt": r.damage_dealt,
            "damage_taken": r.damage_taken,
            "shots_fired":  r.shots_fired,
            "shots_hit":    r.shots_hit,
            "kd_ratio":     round(r.kills / r.deaths, 2) if r.deaths > 0 else r.kills,
        }
        for r in rows
    ]


async def get_leaderboard(db: AsyncSession, limit: int = 10) -> list:
    result = await db.execute(
        select(
            TankStats.tank_id,
            TankStats.team,
            func.sum(TankStats.kills).label("total_kills"),
            func.sum(TankStats.damage_dealt).label("total_damage"),
            func.count(TankStats.battle_id).label("battles_played"),
        )
        .group_by(TankStats.tank_id, TankStats.team)
        .order_by(func.sum(TankStats.kills).desc())
        .limit(limit)
    )
    return [
        {
            "tank_id":       r.tank_id,
            "team":          r.team,
            "total_kills":   r.total_kills,
            "total_damage":  r.total_damage,
            "battles_played": r.battles_played,
        }
        for r in result.all()
    ]
