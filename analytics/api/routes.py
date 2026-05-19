from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from analytics.db import crud
from analytics.db.database import get_db
from analytics.models.battle import Battle

router = APIRouter(prefix="/api/v1")


@router.get("/health")
async def health():
    return {"status": "ok", "service": "analytics"}


@router.get("/battles")
async def list_battles(db: AsyncSession = Depends(get_db)):
    result = await db.execute(
        select(Battle).order_by(Battle.id.desc()).limit(50)
    )
    battles = result.scalars().all()
    return [
        {
            "id":           b.id,
            "started_at":   b.started_at,
            "ended_at":     b.ended_at,
            "winner_team":  b.winner_team,
            "total_ticks":  b.total_ticks,
            "total_events": b.total_events,
        }
        for b in battles
    ]


@router.get("/battles/{battle_id}")
async def get_battle(battle_id: int, db: AsyncSession = Depends(get_db)):
    stats = await crud.get_battle_stats(db, battle_id)
    if stats is None:
        raise HTTPException(status_code=404, detail="Battle not found")
    return stats


@router.get("/tanks/{tank_id}/stats")
async def tank_stats(tank_id: int, db: AsyncSession = Depends(get_db)):
    rows = await crud.get_tank_performance(db, tank_id)
    if not rows:
        raise HTTPException(status_code=404, detail="No stats found for this tank")
    return rows


@router.get("/leaderboard")
async def leaderboard(
    limit: int = Query(default=10, ge=1, le=128),
    db: AsyncSession = Depends(get_db),
):
    return await crud.get_leaderboard(db, limit=limit)
