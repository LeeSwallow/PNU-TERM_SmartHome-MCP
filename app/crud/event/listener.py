import json

import app.crud.dao.sensor as sensor_dao
import app.crud.dao.actuator as actuator_dao
from app.util.logging import logging
from app.model.enums import RegisterType
from app.util.database import SessionLocal
from app.schema.mqtt import (
    MqttRegisterSensorMessage,
    MqttRegisterActuatorMessage,
    MqttUpdateSensorStateMessage,
    MqttUpdateActuatorStateMessage,
    MqttRegisterResponse,
)
from app.crud.event.publisher import send_register_response
from app.crud.event.sse import publish_sse_event

logger = logging.getLogger(__name__)

def _extract_device_code(topic: str) -> str:
    if topic.startswith("devices/"):
        return topic.split("/")[1]
    else:
        raise ValueError(f"잘못된 토픽 형식: {topic}")

def on_register(client, userdata, message):
    session = SessionLocal()
    try:
        device_code = _extract_device_code(message.topic)
        payload = json.loads(message.payload.decode('utf-8'))
        register_type = RegisterType(payload['type'])

        if register_type == RegisterType.SENSOR:
            request = MqttRegisterSensorMessage.model_validate(payload)
            sensor = sensor_dao.register_if_not_exists(session, device_code, request) 
            response = MqttRegisterResponse(type=RegisterType.SENSOR, name=sensor.name) # type: ignore

        elif register_type == RegisterType.ACTUATOR:
            request = MqttRegisterActuatorMessage.model_validate(payload)
            actuator = actuator_dao.register_if_not_exists(session, device_code, request)
            response = MqttRegisterResponse(type=RegisterType.ACTUATOR, name=actuator.name) # type: ignore
        
        else:
            raise ValueError(f"잘못된 등록 타입: {register_type}")
        logger.info(f"등록 완료: {response}")
        send_register_response(client, device_code, response)
        publish_sse_event(device_code, {"event": "register", "data": response.model_dump()})
    except Exception as e:
        logger.error(f"등록 처리 실패: {e}")
        session.rollback()
    finally:
        session.close()


def on_update(client, userdata, message):
    session = SessionLocal()
    try:
        device_code = _extract_device_code(message.topic)
        payload = json.loads(message.payload.decode('utf-8'))
        update_type = RegisterType(payload['type'])

        if update_type == RegisterType.SENSOR:
            request = MqttUpdateSensorStateMessage.model_validate(payload)
            sensor_dao.update_state(session, device_code, request)
            publish_sse_event(device_code, {"event": "update", "data": request.model_dump()})

        elif update_type == RegisterType.ACTUATOR:
            request = MqttUpdateActuatorStateMessage.model_validate(payload)
            actuator_dao.update_state(session, device_code, request)
            publish_sse_event(device_code, {"event": "update", "data": request.model_dump()})
        
        else:
            raise ValueError(f"잘못된 업데이트 타입: {update_type}")
        logger.info(f"업데이트 완료: {request}")
    
    except Exception as e:
        logger.error(f"업데이트 처리 실패: {e}")
        session.rollback()
    finally:
        session.close()
