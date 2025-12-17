from fastapi import APIRouter
from app.util.database import SessionDep
from app.schema.base import PageResponse, DefaultEdit
import app.crud.rest as crud
from app.schema.rest import RestActuatorResponse, RestActuatorUpdateRequest

router = APIRouter(
    prefix="/api/v1/devices/{device_code}/actuators",
    tags=["actuator"]
)

@router.get("/")
def get_actuators(device_code: str, session: SessionDep) -> list[RestActuatorResponse]:
    return crud.get_actuators(session, device_code)

@router.get("/pagination")
def get_pagination_actuators(device_code: str, session: SessionDep, page: int = 1, size: int = 10) -> PageResponse[RestActuatorResponse]:
    return crud.get_pagination_actuators(session, device_code, page, size)

@router.get("/{actuator_id}")
def get_actuator(device_code: str, actuator_id: int, session: SessionDep) -> RestActuatorResponse:
    return crud.get_actuator(session, device_code, actuator_id)

@router.put("/{actuator_id}")
def update_actuator(device_code:str, actuator_id: int, request: DefaultEdit, session: SessionDep) -> RestActuatorResponse:
    return crud.update_actuator(db=session, actuator_id=actuator_id, device_code=device_code, request=request)

@router.post("/{actuator_id}/action")
def update_actuator_state(device_code:str, actuator_id: int, request: RestActuatorUpdateRequest, session: SessionDep):
    return crud.update_actuator_state(db=session, actuator_id=actuator_id, device_code=device_code, request=request)