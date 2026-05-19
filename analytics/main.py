from contextlib import asynccontextmanager

from fastapi import FastAPI

from analytics.api.routes import router
from analytics.worker.event_consumer import EventConsumer

_consumer = EventConsumer()


@asynccontextmanager
async def lifespan(app: FastAPI):
    await _consumer.start()
    yield
    await _consumer.stop()


app = FastAPI(title="TankBattleSim Analytics", version="1.0.0", lifespan=lifespan)
app.include_router(router)
