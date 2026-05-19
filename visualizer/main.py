"""
TankBattleSim Visualizer
Run from the visualizer/ directory:  python main.py
Controls: SPACE=pause  R=restart  ESC=quit
"""
import sys
import pygame

from config import (
    WINDOW_W, WINDOW_H, SIDEBAR_W, BATTLE_W, BOTTOM_H,
    BG_COLOR, YELLOW,
    LOG_PATH, FPS,
)
from data.event_reader import EventReader
from renderer.battlefield import draw_battlefield
from renderer.hud import draw_sidebar, draw_bottom_bar


def main() -> None:
    pygame.init()
    pygame.display.set_caption("TankBattleSim Visualizer — SPACE pause · R restart · ESC quit")
    screen = pygame.display.set_mode((WINDOW_W, WINDOW_H))
    clock  = pygame.time.Clock()

    font_sm = pygame.font.SysFont("monospace", 11)
    font_md = pygame.font.SysFont("monospace", 13)
    font_lg = pygame.font.SysFont("monospace", 22, bold=True)

    reader = EventReader(LOG_PATH)

    # Off-screen surfaces for each region
    battle_surf  = pygame.Surface((BATTLE_W,  WINDOW_H - BOTTOM_H))
    sidebar_surf = pygame.Surface((SIDEBAR_W, WINDOW_H - BOTTOM_H))
    bottom_surf  = pygame.Surface((WINDOW_W,  BOTTOM_H))

    paused  = False
    running = True

    while running:
        clock.tick(FPS)
        fps = clock.get_fps()

        # ── Events ────────────────────────────────────────────────────────────
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key == pygame.K_SPACE:
                    paused = not paused
                elif event.key == pygame.K_r:
                    reader.restart()
                    paused = False

        # ── Sim data ──────────────────────────────────────────────────────────
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
        screen.blit(battle_surf,  (0,         0))
        screen.blit(sidebar_surf, (BATTLE_W,  0))
        screen.blit(bottom_surf,  (0,         WINDOW_H - BOTTOM_H))

        # Winner banner (centred over battle area)
        winner = reader.winner
        if winner and not waiting:
            colour  = (80, 160, 255) if "A" in winner else (255, 80, 80) if "B" in winner else YELLOW
            banner  = font_lg.render(f"  {winner} wins!  ", True, colour)
            bx      = BATTLE_W // 2 - banner.get_width() // 2
            by      = (WINDOW_H - BOTTOM_H) // 2 - banner.get_height() // 2
            bg_rect = banner.get_rect(topleft=(bx - 4, by - 4)).inflate(8, 8)
            pygame.draw.rect(screen, (10, 10, 20), bg_rect, border_radius=4)
            pygame.draw.rect(screen, colour, bg_rect, 1, border_radius=4)
            screen.blit(banner, (bx, by))

        pygame.display.flip()

    reader.close()
    pygame.quit()
    sys.exit(0)


if __name__ == "__main__":
    main()
