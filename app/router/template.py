from typing import Optional
from fastapi import APIRouter, Request
from fastapi.templating import Jinja2Templates
from fastapi.responses import HTMLResponse
from app.util.database import SessionDep
import app.crud.rest as crud
from app.schema.rest import RestDeviceResponse

router = APIRouter(
    tags=["html"]
)

templates: Optional[Jinja2Templates] = None

def set_templates(templates_instance: Jinja2Templates):
    global templates
    templates = templates_instance

@router.get("/", response_class=HTMLResponse)
def get_index(request: Request, session: SessionDep):
    if templates is None:
        raise RuntimeError("템플릿이 초기화되지 않았습니다. server.py에서 set_templates()를 호출하세요.")
    devices: list[RestDeviceResponse] = crud.get_devices(session)
    return templates.TemplateResponse("index.html", {
        "request": request, "devices": devices
    })

@router.get("/device/{device_code}", response_class=HTMLResponse)
def get_device_page(request: Request, device_code: str, session: SessionDep):
    """기기 상세 페이지 렌더링"""
    if templates is None:
        raise RuntimeError("템플릿이 초기화되지 않았습니다. server.py에서 set_templates()를 호출하세요.")
    
    # 서버에서 기기 정보, 액추에이터, 센서 가져오기
    device = crud.get_device_by_code(session, device_code)
    actuators = crud.get_actuators(session, device_code)
    sensors = crud.get_sensors(session, device_code)
    
    return templates.TemplateResponse("device.html", {
        "request": request,
        "device": device,
        "actuators": actuators,
        "sensors": sensors
    })
