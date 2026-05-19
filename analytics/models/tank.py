from sqlalchemy import Integer, String, UniqueConstraint
from sqlalchemy.orm import Mapped, mapped_column

from analytics.db.database import Base


class TankStats(Base):
    __tablename__ = "tank_stats"
    __table_args__ = (UniqueConstraint("tank_id", "battle_id", name="uq_tank_battle"),)

    id:            Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    tank_id:       Mapped[int] = mapped_column(Integer, index=True)
    battle_id:     Mapped[int] = mapped_column(Integer, index=True)
    team:          Mapped[str] = mapped_column(String(1))
    kills:         Mapped[int] = mapped_column(Integer, default=0)
    deaths:        Mapped[int] = mapped_column(Integer, default=0)
    damage_dealt:  Mapped[int] = mapped_column(Integer, default=0)
    damage_taken:  Mapped[int] = mapped_column(Integer, default=0)
    shots_fired:   Mapped[int] = mapped_column(Integer, default=0)
    shots_hit:     Mapped[int] = mapped_column(Integer, default=0)
