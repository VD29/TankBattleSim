# TankBattleSim

A tank battle simulator composed of three independent services that communicate
via newline-delimited JSON events in `shared/battle_log.jsonl`.

## Components

| Component | Language | Purpose |
|-----------|----------|---------|
| `cpp_sim/` | C++17 | ECS game engine — runs the simulation |
| `visualizer/` | Python / pygame | Real-time battle visualization |
| `analytics/` | Python / FastAPI | Event ingestion, storage, REST API |

## Prerequisites

- CMake ≥ 3.20, a C++17 compiler (clang++ or g++)
- Python 3.11+
- Docker & Docker Compose

## Quick start

### 1. Infrastructure

```bash
docker compose up -d
```

PostgreSQL 15 → `localhost:5432` (db/user/pass: `tanksim` / `tanksim` / `tanksim_secret`)  
RabbitMQ 3 → AMQP `localhost:5672`, Management UI `http://localhost:15672`

### 2. C++ engine

```bash
cd cpp_sim
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/tank_sim
```

Run tests:

```bash
cd build && ctest --output-on-failure
```

### 3. Visualizer

```bash
cd visualizer
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

### 4. Analytics service

```bash
cd analytics
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
uvicorn main:app --reload --port 8000
```

## Architecture

```
cpp_sim  ──writes──▶  shared/battle_log.jsonl  ◀──reads──  visualizer
                                │
                                └──reads──  analytics worker  ──▶  PostgreSQL / RabbitMQ
```

All events are JSON objects, one per line, with at minimum:
```json
{"timestamp": 1.23, "type": "tank_move", "entity_id": 42, ...}
```
