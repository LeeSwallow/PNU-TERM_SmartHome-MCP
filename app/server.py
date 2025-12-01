import os

from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from contextlib import asynccontextmanager
from app.router import device, actuator, sensor, template
from app.util.database import init_db
from app.util.logging import logging
from app.util.broker import mqttClient
from app.crud.event.listener import on_register, on_update
from fastapi.templating import Jinja2Templates
from prometheus_fastapi_instrumentator import Instrumentator

logger = logging.getLogger(__name__)

def mqtt_on_connect(client, userdata, flags, rc):
    if rc == 0:
        logger.info("MQTT 브로커 연결 성공")
        
        # 연결 성공 후 토픽 구독
        client.subscribe("devices/+/register")
        client.subscribe("devices/+/update")
        logger.info("MQTT 토픽 구독 완료")
    else:
        logger.error("MQTT 브로커 연결 실패: %s", rc)
        client.reconnect()


@asynccontextmanager
async def lifespan(app: FastAPI):
    # 서버 시작 시
    init_db()
    logger.info("데이터베이스 초기화 완료")
    
    mqttClient.on_connect = mqtt_on_connect
    
    # 메시지 콜백 등록 (loop_start 전에 등록)
    mqttClient.message_callback_add("devices/+/register", on_register)
    mqttClient.message_callback_add("devices/+/update", on_update)
    
    # MQTT 네트워크 루프 시작 (연결 시작)
    mqttClient.loop_start()
    yield
    # 서버 종료 시
    mqttClient.loop_stop()
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
