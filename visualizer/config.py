import pathlib

# ── Window layout ─────────────────────────────────────────────────────────────
WINDOW_W  = 900
WINDOW_H  = 600
SIDEBAR_W = 220
BOTTOM_H  = 30
BATTLE_W  = WINDOW_W - SIDEBAR_W   # 680
BATTLE_H  = WINDOW_H - BOTTOM_H    # 570

# ── Tank grid (64 tanks × 2 teams — no position data in log, use fixed grid) ─
TANKS_PER_TEAM  = 64
FIRST_TEAM_B_ID = 64
GRID_COLS       = 8
GRID_ROWS       = 8
TANK_W          = 10
TANK_H          = 10

# Team A grid bounds (pixels inside battle area)
A_GRID_X0 = 15
A_GRID_X1 = 280
A_GRID_Y0 = 15
A_GRID_Y1 = BATTLE_H - 15   # 555

# Team B grid bounds
B_GRID_X0 = BATTLE_W - 280  # 400
B_GRID_X1 = BATTLE_W - 15   # 665

# ── Dark theme palette ────────────────────────────────────────────────────────
BG_COLOR     = (12,  12,  22)
BATTLE_BG    = (18,  18,  32)
SIDEBAR_BG   = (22,  22,  38)
BOTTOM_BG    = (15,  15,  28)
DIVIDER      = (35,  35,  55)

TEAM_A_COLOR = (60,  130, 220)   # blue
TEAM_B_COLOR = (220,  60,  60)   # red
DEAD_COLOR   = (65,   65,  75)   # gray

HP_A_BG      = (25,  55,  25)
HP_A_FILL    = (55,  175,  90)   # green
HP_B_BG      = (60,  25,  25)
HP_B_FILL    = (215, 135,  35)   # orange
HP_DEAD      = (55,   55,  65)

TEXT_COLOR   = (200, 200, 215)
DIM_COLOR    = (100, 100, 120)
ACCENT_A     = (80,  160, 255)
ACCENT_B     = (255,  80,  80)
YELLOW       = (255, 215,  50)

# ── Simulation constants (must match C++) ─────────────────────────────────────
TANK_MAX_HP = 100
SIM_DT      = 0.016          # seconds per tick

# ── Paths ─────────────────────────────────────────────────────────────────────
LOG_PATH = pathlib.Path(__file__).parent.parent / "shared" / "battle_log.jsonl"

# ── Display ───────────────────────────────────────────────────────────────────
FPS             = 60
EVENT_LOG_LINES = 10
