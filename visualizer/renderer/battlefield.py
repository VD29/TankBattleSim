import pygame

from config import (
    BATTLE_W, BATTLE_H,
    BATTLE_BG, DIVIDER,
    TEAM_A_COLOR, TEAM_B_COLOR, DEAD_COLOR,
    TANK_W, TANK_H,
    GRID_COLS, GRID_ROWS,
    TANKS_PER_TEAM,
    A_GRID_X0, A_GRID_X1, A_GRID_Y0, A_GRID_Y1,
    B_GRID_X0, B_GRID_X1,
    TEXT_COLOR, DIM_COLOR,
)
from data.event_reader import EventReader


def _build_grid_positions() -> dict:
    """Pre-compute fixed pixel positions for all 128 tanks (8×8 grids per team)."""
    positions: dict = {}

    col_step_a = (A_GRID_X1 - A_GRID_X0) / max(GRID_COLS - 1, 1)
    row_step   = (A_GRID_Y1 - A_GRID_Y0) / max(GRID_ROWS - 1, 1)
    col_step_b = (B_GRID_X1 - B_GRID_X0) / max(GRID_COLS - 1, 1)

    for i in range(TANKS_PER_TEAM):
        row, col = divmod(i, GRID_COLS)
        positions[i] = (
            int(A_GRID_X0 + col * col_step_a),
            int(A_GRID_Y0 + row * row_step),
        )

    for j in range(TANKS_PER_TEAM):
        row, col = divmod(j, GRID_COLS)
        positions[TANKS_PER_TEAM + j] = (
            int(B_GRID_X0 + col * col_step_b),
            int(A_GRID_Y0 + row * row_step),
        )

    return positions


_GRID = _build_grid_positions()

# White border rect used for alive tanks
_BORDER = (255, 255, 255)


def draw_battlefield(surface: pygame.Surface,
                     reader: EventReader,
                     waiting: bool,
                     font: pygame.font.Font) -> None:
    surface.fill(BATTLE_BG)

    # Centre dividing line
    cx = BATTLE_W // 2
    pygame.draw.line(surface, DIVIDER, (cx, 0), (cx, BATTLE_H), 1)

    # Team labels at top
    label_a = font.render("TEAM A", True, TEAM_A_COLOR)
    label_b = font.render("TEAM B", True, TEAM_B_COLOR)
    surface.blit(label_a, (A_GRID_X0, 2))
    surface.blit(label_b, (B_GRID_X0, 2))

    if waiting:
        msg = font.render("Waiting for simulator…", True, TEXT_COLOR)
        surface.blit(msg, (cx - msg.get_width() // 2, BATTLE_H // 2))
        hint = font.render("Run: ./cpp_sim/build/tank_sim", True, DIM_COLOR)
        surface.blit(hint, (cx - hint.get_width() // 2, BATTLE_H // 2 + 18))
        return

    for entity_id, tank in reader.tanks.items():
        pos = _GRID.get(entity_id)
        if pos is None:
            continue

        if not tank.alive:
            color = DEAD_COLOR
        elif tank.team == "A":
            color = TEAM_A_COLOR
        else:
            color = TEAM_B_COLOR

        rect = pygame.Rect(pos[0], pos[1], TANK_W, TANK_H)
        pygame.draw.rect(surface, color, rect)
        if tank.alive:
            pygame.draw.rect(surface, _BORDER, rect, 1)
