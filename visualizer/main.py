"""
TankBattleSim Visualizer
Run from the visualizer/ directory:  python main.py
Controls: SPACE=pause  R=restart  ESC=quit  RAND=new random battle
"""
import pathlib
import random
import subprocess
import sys
import time
import threading

import pygame

from config import (
    WINDOW_W, WINDOW_H, SIDEBAR_W, BATTLE_W, BOTTOM_H,
    BG_COLOR, YELLOW, ACCENT_A, ACCENT_B,
    LOG_PATH, FPS,
)
from data.event_reader import EventReader
from renderer.battlefield import draw_battlefield
from renderer.hud import draw_sidebar, draw_bottom_bar

_SIM_BIN = pathlib.Path(__file__).parent.parent / "cpp_sim" / "build" / "tank_sim"

# ── Bottom-bar RAND button layout ─────────────────────────────────────────────
_SEED_LABEL_X = BATTLE_W + 4
_BTN_W        = 44
_BTN_H        = 18
_RAND_X       = _SEED_LABEL_X

_INPUT_BORDER = (100, 100, 140)
_BTN_COLOR    = (45, 65, 100)
_BTN_HOVER    = (65, 95, 140)
_BTN_TEXT     = (200, 220, 255)


def _screen_rect_for_bar_widget(x: int, w: int, h: int) -> pygame.Rect:
    bar_top = WINDOW_H - BOTTOM_H
    y = bar_top + (BOTTOM_H - h) // 2
    return pygame.Rect(x, y, w, h)


def _launch_sim(seed: int, done_event: threading.Event) -> None:
    try:
        subprocess.run(
            [str(_SIM_BIN), "--seed", str(seed)],
            cwd=str(_SIM_BIN.parent),   # log path in main.cpp is relative to build dir
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except FileNotFoundError:
        pass
    done_event.set()


def main() -> None:
    pygame.init()
    pygame.display.set_caption("TankBattleSim Visualizer — SPACE pause · R restart · ESC quit")
    screen = pygame.display.set_mode((WINDOW_W, WINDOW_H))
    clock  = pygame.time.Clock()

    font_sm = pygame.font.SysFont("monospace", 11)
    font_md = pygame.font.SysFont("monospace", 13)
    font_lg = pygame.font.SysFont("monospace", 22, bold=True)

    reader = EventReader(LOG_PATH)

    battle_surf  = pygame.Surface((BATTLE_W,  WINDOW_H - BOTTOM_H))
    sidebar_surf = pygame.Surface((SIDEBAR_W, WINDOW_H - BOTTOM_H))
    bottom_surf  = pygame.Surface((WINDOW_W,  BOTTOM_H))

    current_seed = random.randint(0, 999999)
    simulating   = False
    sim_done_evt: threading.Event | None = None
    paused       = False
    running      = True

    rand_rect = _screen_rect_for_bar_widget(_RAND_X, _BTN_W, _BTN_H)

    def start_sim(seed: int) -> None:
        nonlocal simulating, sim_done_evt, current_seed, paused
        current_seed = seed
        evt = threading.Event()
        sim_done_evt = evt
        simulating = True
        paused = False
        threading.Thread(target=_launch_sim, args=(seed, evt), daemon=True).start()

    while running:
        clock.tick(FPS)
        fps  = clock.get_fps()
        mpos = pygame.mouse.get_pos()

        # ── Check if sim finished ─────────────────────────────────────────────
        if simulating and sim_done_evt is not None and sim_done_evt.is_set():
            simulating   = False
            sim_done_evt = None
            reader.close()
            time.sleep(0.1)
            reader = EventReader(LOG_PATH)
            reader._load_file()
            first_x = next(
                (e.get("x") for _, e in reader._buffer if e.get("type") == "tank_moved"),
                None,
            )
            print(f"Reloaded log: {reader.total_events} events, first x={first_x}")

        # ── Events ────────────────────────────────────────────────────────────
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                if rand_rect.collidepoint(mpos) and not simulating:
                    start_sim(random.randint(0, 999999))

            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key == pygame.K_SPACE and not simulating:
                    paused = not paused
                elif event.key == pygame.K_r and not simulating:
                    reader.restart()
                    paused = False

        # ── Sim data ──────────────────────────────────────────────────────────
        if not simulating:
            if not paused:
                reader.advance()
            else:
                reader.poll()

        waiting = not LOG_PATH.exists()

        # ── Render ────────────────────────────────────────────────────────────
        draw_battlefield(battle_surf, reader, waiting, font_sm)
        draw_sidebar(sidebar_surf, reader, font_sm, font_md)
        draw_bottom_bar(bottom_surf, reader, paused, fps, font_sm)

        screen.fill(BG_COLOR)
        screen.blit(battle_surf,  (0,        0))
        screen.blit(sidebar_surf, (BATTLE_W, 0))
        screen.blit(bottom_surf,  (0,        WINDOW_H - BOTTOM_H))

        # ── RAND button + seed label ──────────────────────────────────────────
        bar_top  = WINDOW_H - BOTTOM_H
        rand_col = _BTN_HOVER if rand_rect.collidepoint(mpos) and not simulating else _BTN_COLOR
        pygame.draw.rect(screen, rand_col,      rand_rect)
        pygame.draw.rect(screen, _INPUT_BORDER, rand_rect, 1)
        rand_lbl = font_sm.render("RAND", True, _BTN_TEXT)
        screen.blit(rand_lbl, (rand_rect.x + (rand_rect.w - rand_lbl.get_width()) // 2,
                                rand_rect.y + (rand_rect.h - rand_lbl.get_height()) // 2))

        seed_surf = font_sm.render(f"#{current_seed}", True, (150, 150, 180))
        screen.blit(seed_surf, (_RAND_X + _BTN_W + 5,
                                  bar_top + (BOTTOM_H - seed_surf.get_height()) // 2))

        # ── Winner banner ─────────────────────────────────────────────────────
        winner = reader.winner
        if winner and not waiting and not simulating:
            colour  = ACCENT_A if "A" in winner else ACCENT_B if "B" in winner else YELLOW
            banner  = font_lg.render(f"  {winner} wins!  ", True, colour)
            bx      = BATTLE_W // 2 - banner.get_width() // 2
            by      = (WINDOW_H - BOTTOM_H) // 2 - banner.get_height() // 2
            bg_rect = banner.get_rect(topleft=(bx - 4, by - 4)).inflate(8, 8)
            pygame.draw.rect(screen, (10, 10, 20), bg_rect, border_radius=4)
            pygame.draw.rect(screen, colour, bg_rect, 1, border_radius=4)
            screen.blit(banner, (bx, by))

        # ── Simulating overlay ────────────────────────────────────────────────
        if simulating:
            overlay = pygame.Surface((BATTLE_W, WINDOW_H - BOTTOM_H), pygame.SRCALPHA)
            overlay.fill((0, 0, 0, 160))
            screen.blit(overlay, (0, 0))
            msg  = font_lg.render("Simulating…", True, YELLOW)
            hint = font_sm.render(f"seed #{current_seed}", True, (180, 180, 180))
            mx   = BATTLE_W // 2 - msg.get_width()  // 2
            my   = (WINDOW_H - BOTTOM_H) // 2 - msg.get_height() // 2
            screen.blit(msg,  (mx, my))
            screen.blit(hint, (BATTLE_W // 2 - hint.get_width() // 2, my + msg.get_height() + 6))

        pygame.display.flip()

    reader.close()
    pygame.quit()
    sys.exit(0)


if __name__ == "__main__":
    main()
