import pygame

from config import (
    BATTLE_W, BATTLE_H,
    BATTLE_BG, DIVIDER,
    TEAM_A_COLOR, TEAM_B_COLOR, DEAD_COLOR,
    TANK_W, TANK_H,
    TEXT_COLOR, DIM_COLOR,
)
from data.event_reader import EventReader

# Sim world dimensions (must match C++ constants)
_SIM_W = 800.0
_SIM_H = 600.0

_SCALE_X = BATTLE_W / _SIM_W
_SCALE_Y = BATTLE_H / _SIM_H

_BORDER = (255, 255, 255)


def _sim_to_screen(x: float, y: float) -> tuple:
    return (
        int(x * _SCALE_X) - TANK_W // 2,
        int(y * _SCALE_Y) - TANK_H // 2,
    )


def draw_battlefield(surface: pygame.Surface,
                     reader: EventReader,
                     waiting: bool,
                     font: pygame.font.Font) -> None:
    surface.fill(BATTLE_BG)

    # Subtle dividing line at the sim mid-point (x=400 → screen x=340)
    cx = int(400.0 * _SCALE_X)
    pygame.draw.line(surface, DIVIDER, (cx, 0), (cx, BATTLE_H), 1)

    # Team labels
    surface.blit(font.render("TEAM A", True, TEAM_A_COLOR), (8, 2))
    surface.blit(font.render("TEAM B", True, TEAM_B_COLOR), (BATTLE_W - 52, 2))

    if waiting:
        msg  = font.render("Waiting for simulator…", True, TEXT_COLOR)
        hint = font.render("Run: ./cpp_sim/build/tank_sim", True, DIM_COLOR)
        surface.blit(msg,  (BATTLE_W // 2 - msg.get_width()  // 2, BATTLE_H // 2))
        surface.blit(hint, (BATTLE_W // 2 - hint.get_width() // 2, BATTLE_H // 2 + 18))
        return

    for tank in reader.tanks.values():
        if not tank.has_position:
            continue

        color = DEAD_COLOR if not tank.alive else (
            TEAM_A_COLOR if tank.team == "A" else TEAM_B_COLOR
        )
        sx, sy = _sim_to_screen(tank.x, tank.y)
        rect = pygame.Rect(sx, sy, TANK_W, TANK_H)
        pygame.draw.rect(surface, color, rect)
        if tank.alive:
            pygame.draw.rect(surface, _BORDER, rect, 1)
