import pygame

from config import (
    SIDEBAR_W, WINDOW_H, BOTTOM_H, WINDOW_W, BATTLE_W,
    SIDEBAR_BG, BOTTOM_BG, DIVIDER,
    TEAM_A_COLOR, TEAM_B_COLOR,
    HP_A_BG, HP_A_FILL, HP_B_BG, HP_B_FILL, HP_DEAD,
    TEXT_COLOR, DIM_COLOR, ACCENT_A, ACCENT_B, YELLOW,
    TANKS_PER_TEAM, TANK_MAX_HP, SIM_DT,
    EVENT_LOG_LINES,
)
from data.event_reader import EventReader

_SIDEBAR_H = WINDOW_H - BOTTOM_H   # 570

# Layout constants (all in local sidebar coords)
_PAD      = 5
_BAR_H    = 3    # HP bar height in pixels
_BAR_GAP  = 0    # gap between bars
_BAR_W    = SIDEBAR_W - 10   # 210px

_SECTION_LABEL_H = 16


def draw_sidebar(surface: pygame.Surface,
                 reader: EventReader,
                 font_sm: pygame.font.Font,
                 font_md: pygame.font.Font) -> None:
    surface.fill(SIDEBAR_BG)

    # Vertical right-edge separator
    pygame.draw.line(surface, DIVIDER, (0, 0), (0, _SIDEBAR_H), 1)

    y = _PAD

    # ── Team A section ────────────────────────────────────────────────────────
    alive_a = reader.alive_a
    lbl_a   = font_md.render(f"■ Team A  {alive_a:>2}/{TANKS_PER_TEAM}", True, ACCENT_A)
    surface.blit(lbl_a, (_PAD, y))
    y += _SECTION_LABEL_H

    tanks_a = [t for t in reader.tanks.values() if t.team == "A"]
    tanks_a.sort(key=lambda t: t.entity_id)
    for tank in tanks_a:
        _draw_hp_bar(surface, _PAD, y, tank.hp, tank.max_hp, tank.alive,
                     HP_A_BG, HP_A_FILL)
        y += _BAR_H + _BAR_GAP

    y += _PAD

    # ── Team B section ────────────────────────────────────────────────────────
    alive_b = reader.alive_b
    lbl_b   = font_md.render(f"■ Team B  {alive_b:>2}/{TANKS_PER_TEAM}", True, ACCENT_B)
    surface.blit(lbl_b, (_PAD, y))
    y += _SECTION_LABEL_H

    tanks_b = [t for t in reader.tanks.values() if t.team == "B"]
    tanks_b.sort(key=lambda t: t.entity_id)
    for tank in tanks_b:
        _draw_hp_bar(surface, _PAD, y, tank.hp, tank.max_hp, tank.alive,
                     HP_B_BG, HP_B_FILL)
        y += _BAR_H + _BAR_GAP

    y += _PAD

    # ── Event log ─────────────────────────────────────────────────────────────
    pygame.draw.line(surface, DIVIDER, (_PAD, y), (SIDEBAR_W - _PAD, y), 1)
    y += 3
    evlbl = font_md.render("Events", True, TEXT_COLOR)
    surface.blit(evlbl, (_PAD, y))
    y += _SECTION_LABEL_H

    recent = reader.recent_events
    for line in recent:
        color = DIM_COLOR
        if "destroyed" in line:
            color = YELLOW
        elif "→" in line:
            color = TEXT_COLOR
        txt = font_sm.render(line, True, color)
        surface.blit(txt, (_PAD, y))
        y += font_sm.get_height() + 1


def draw_bottom_bar(surface: pygame.Surface,
                    reader: EventReader,
                    paused: bool,
                    fps: float,
                    font: pygame.font.Font) -> None:
    surface.fill(BOTTOM_BG)
    pygame.draw.line(surface, DIVIDER, (0, 0), (WINDOW_W, 0), 1)

    sim_time = reader.tick * SIM_DT
    left = (f"Tick {reader.tick:>5}  |  Sim {sim_time:>6.1f}s  |  "
            f"Kills A:{reader.kills_a}  B:{reader.kills_b}")
    left_surf = font.render(left, True, TEXT_COLOR)
    surface.blit(left_surf, (6, (BOTTOM_BAR_H := surface.get_height()) // 2 - left_surf.get_height() // 2))

    winner = reader.winner
    if winner:
        centre_txt = f"⚑  {winner} wins!"
        colour     = ACCENT_A if "A" in winner else ACCENT_B if "B" in winner else YELLOW
        c_surf = font.render(centre_txt, True, colour)
        surface.blit(c_surf, (BATTLE_W // 2 - c_surf.get_width() // 2,
                               BOTTOM_BAR_H // 2 - c_surf.get_height() // 2))
    elif paused:
        p_surf = font.render("⏸  PAUSED", True, YELLOW)
        surface.blit(p_surf, (BATTLE_W // 2 - p_surf.get_width() // 2,
                               BOTTOM_BAR_H // 2 - p_surf.get_height() // 2))

    fps_str  = f"FPS {fps:>4.0f}"
    fps_surf = font.render(fps_str, True, DIM_COLOR)
    surface.blit(fps_surf, (WINDOW_W - fps_surf.get_width() - 6,
                             BOTTOM_BAR_H // 2 - fps_surf.get_height() // 2))


# ── Helpers ───────────────────────────────────────────────────────────────────

def _draw_hp_bar(surface: pygame.Surface,
                 x: int, y: int,
                 hp: int, max_hp: int,
                 alive: bool,
                 bg_color: tuple, fill_color: tuple) -> None:
    bg_rect = pygame.Rect(x, y, _BAR_W, _BAR_H)
    pygame.draw.rect(surface, bg_color, bg_rect)
    if max_hp > 0:
        fill_w = int(_BAR_W * hp / max_hp)
        if fill_w > 0:
            fill = fill_color if alive else HP_DEAD
            pygame.draw.rect(surface, fill, pygame.Rect(x, y, fill_w, _BAR_H))
