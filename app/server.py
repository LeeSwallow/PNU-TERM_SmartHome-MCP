import os
import asyncio

from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from contextlib import asynccontextmanager

from app.router import device, actuator, sensor, template
from app.util.database import init_db
from app.util.logging import logging
from app.util import broker
from app.crud.event.sse import set_event_loop

from fastapi.templating import Jinja2Templates
from prometheus_fastapi_instrumentator import Instrumentator

logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):
    # 서버 시작 시
    init_db()
    logger.info("데이터베이스 초기화 완료")
    
    # MQTT 네트워크 루프 시작 (연결 시작)
    broker.get_mqtt_client()
    broker.mqttClient.loop_start()
    # 현재 이벤트 루프를 SSE 퍼블리셔에 등록 (타 스레드에서 이벤트 전달용)
    set_event_loop(asyncio.get_running_loop())
    yield
    # 서버 종료 시
    if broker.mqttClient:
        broker.mqttClient.loop_stop()
        logger.info("MQTT 브로커 연결 종료")

    
app = FastAPI(
    title="IoT Home Server",
    description="IoT Home Server API",
    version="0.1.0",
    lifespan=lifespan
)

# Prometheus Metric 등록
Instrumentator().instrument(app).expose(app)

# Jinja2 템플릿 설정
templates = Jinja2Templates(directory="app/templates")
app.templates = templates  # type: ignore

# 템플릿 라우터에 템플릿 인스턴스 설정
template.set_templates(templates)

app.include_router(device.router)
app.include_router(actuator.router)
app.include_router(sensor.router)
app.include_router(template.router)

# Template components JS files
TEMPLATES_DIR = os.path.join(os.path.dirname(__file__), "templates")
STATIC_DIR = os.path.join(TEMPLATES_DIR, "static")

if os.path.exists(STATIC_DIR):
    app.mount("/templates/static", StaticFiles(directory=STATIC_DIR), name="static")

# favicon.ico 파일 제공
@app.get("/favicon.ico")
async def read_favicon():
    return FileResponse(os.path.join(STATIC_DIR, "favicon.ico"))
