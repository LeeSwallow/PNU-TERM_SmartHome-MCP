from fastapi import HTTPException
from sqlalchemy.orm import Session
from app.schema.rest import RestDeviceResponse, RestActuatorResponse, RestSensorResponse, RestActuatorUpdateRequest
from app.util import broker
from app.schema.base import PageResponse, DefaultEdit
import app.crud.dao.device as device_dao
import app.crud.dao.actuator as actuator_dao
import app.crud.dao.sensor as sensor_dao
import app.crud.event.publisher as publisher


def get_device_by_code(db: Session, device_code: str) -> RestDeviceResponse:
    device = device_dao.get_device_by_code(db, device_code)
    return RestDeviceResponse.model_validate(device)

def get_devices(db: Session) -> list[RestDeviceResponse]:
    devices = device_dao.get_all(db)
    return [RestDeviceResponse.model_validate(device) for device in devices]

def get_actuators(db: Session, device_code: str) -> list[RestActuatorResponse]:
    actuators = actuator_dao.get_all_by_device_code(db, device_code)
    return [RestActuatorResponse.model_validate(actuator) for actuator in actuators]

def get_actuator(db: Session, device_code: str, actuator_id: int) -> RestActuatorResponse:
    if not actuator_dao.exists_by_device_code_and_id(db, device_code, actuator_id):
        raise HTTPException(status_code=404, detail="해당 액추에이터를 찾을 수 없습니다.")
    actuator = actuator_dao.get_actuator_by_id(db, actuator_id)
    return RestActuatorResponse.model_validate(actuator)

def get_sensors(db: Session, device_code: str) -> list[RestSensorResponse]:
    sensors = sensor_dao.get_all_by_device_code(db, device_code)
    return [RestSensorResponse.model_validate(sensor) for sensor in sensors]

def get_sensor(db: Session, device_code: str, sensor_id: int) -> RestSensorResponse:
    if not sensor_dao.exists_by_device_code_and_id(db, device_code, sensor_id):
        raise HTTPException(status_code=404, detail="해당 센서를 찾을 수 없습니다.")
    sensor = sensor_dao.get_sensor_by_id(db, sensor_id)
    return RestSensorResponse.model_validate(sensor)

def get_pagination_devices(db: Session, page: int, size: int) -> PageResponse[RestDeviceResponse]:
    devices = device_dao.get_pagination(db, page, size)
    return PageResponse[RestDeviceResponse](
        contents=[RestDeviceResponse.model_validate(item) for item in devices.contents],
        page=devices.page,
        size=devices.size,
        total_pages=devices.total_pages,
        total_items=devices.total_items
    )

def get_pagination_actuators(db: Session, device_code: str, page: int, size: int) -> PageResponse[RestActuatorResponse]:
    actuators = actuator_dao.get_pagination(db, device_code, page, size)
    return PageResponse[RestActuatorResponse](
        contents=[RestActuatorResponse.model_validate(item) for item in actuators.contents],
        page=actuators.page,
        size=actuators.size,
        total_pages=actuators.total_pages,
        total_items=actuators.total_items
    )

def get_pagination_sensors(db: Session, device_code: str, page: int, size: int) -> PageResponse[RestSensorResponse]:
    sensors = sensor_dao.get_pagination(db, device_code, page, size)
    return PageResponse[RestSensorResponse](
        contents=[RestSensorResponse.model_validate(item) for item in sensors.contents],
        page=sensors.page,
        size=sensors.size,
        total_pages=sensors.total_pages,
        total_items=sensors.total_items
    )

def update_device(db: Session, device_code: str, request: DefaultEdit) -> RestDeviceResponse:
    device = device_dao.update_detail(db, device_code, request)
    return RestDeviceResponse.model_validate(device)

def update_actuator(db: Session, actuator_id: int, device_code: str, request: DefaultEdit) -> RestActuatorResponse:
    if not actuator_dao.exists_by_device_code_and_id(db, device_code, actuator_id):
        raise HTTPException(status_code=404, detail="해당 액추에이터를 찾을 수 없습니다.")
    actuator = actuator_dao.update_detail(db, actuator_id, request)
    return RestActuatorResponse.model_validate(actuator)

def update_actuator_state(db: Session, actuator_id: int, device_code: str, request: RestActuatorUpdateRequest):
    if not actuator_dao.exists_by_device_code_and_id(db, device_code, actuator_id):
        raise HTTPException(status_code=404, detail="해당 액추에이터를 찾을 수 없습니다.")
    publisher.send_actuator_action(broker.mqttClient, device_code, actuator_id, request.state)

def update_sensor(db: Session, sensor_id: int, device_code: str, request: DefaultEdit) -> RestSensorResponse:
    if not sensor_dao.exists_by_device_code_and_id(db, device_code, sensor_id):
        raise HTTPException(status_code=404, detail="해당 센서를 찾을 수 없습니다.")
    sensor = sensor_dao.update_detail(db, sensor_id, request)
    return RestSensorResponse.model_validate(sensor)