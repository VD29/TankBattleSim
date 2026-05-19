from datetime import datetime
from typing import Optional

from sqlalchemy import DateTime, Float, Integer, String, func
from sqlalchemy.orm import Mapped, mapped_column

from analytics.db.database import Base


class Battle(Base):
    __tablename__ = "battles"

    id:           Mapped[int]               = mapped_column(Integer, primary_key=True, autoincrement=True)
    started_at:   Mapped[datetime]          = mapped_column(DateTime(timezone=True), server_default=func.now())
    ended_at:     Mapped[Optional[datetime]]= mapped_column(DateTime(timezone=True), nullable=True)
    winner_team:  Mapped[Optional[str]]     = mapped_column(String(10),  nullable=True)
    total_ticks:  Mapped[Optional[int]]     = mapped_column(Integer,     nullable=True)
    total_events: Mapped[int]               = mapped_column(Integer,     default=0)


class BattleEvent(Base):
    __tablename__ = "battle_events"

    id:         Mapped[int]             = mapped_column(Integer, primary_key=True, autoincrement=True)
    battle_id:  Mapped[int]             = mapped_column(Integer, index=True)
    event_type: Mapped[str]             = mapped_column(String(50))
    entity_id:  Mapped[Optional[int]]   = mapped_column(Integer, nullable=True)
    target_id:  Mapped[Optional[int]]   = mapped_column(Integer, nullable=True)
    damage:     Mapped[Optional[int]]   = mapped_column(Integer, nullable=True)
    x:          Mapped[Optional[float]] = mapped_column(Float,   nullable=True)
    y:          Mapped[Optional[float]] = mapped_column(Float,   nullable=True)
    tick:       Mapped[Optional[int]]   = mapped_column(Integer, nullable=True)
    timestamp:  Mapped[datetime]        = mapped_column(DateTime(timezone=True), server_default=func.now())
