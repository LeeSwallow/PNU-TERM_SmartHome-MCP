from typing import Annotated
from pydantic import Field
from shmcp.server import mcpServer
from shmcp.schema import ActuatorResponse, SensorResponse

from shmcp.util.client import (
    get_device_info as api_get_device_info,
    api_get_actuators,
    api_get_actuator,
    api_get_sensors,
    api_get_sensor,
    api_update_actuator_state,
    api_update_device_info,
    api_update_actuator_info,
    api_update_sensor_info,
)

@mcpServer.tool(
    name="get_device_info",
    description="IoT 기기 정보를 조회합니다.",
)
async def get_device_info() -> dict:
    device = await api_get_device_info()
    return device.model_dump()

@mcpServer.tool(
    name="list_actuators",
    description="액추에이터 목록을 조회합니다.",
)
async def list_actuators() -> list[dict]:
    data = await api_get_actuators()
    return [ActuatorResponse.model_validate(item).model_dump() for item in data]

@mcpServer.tool(
    name="get_actuator",
    description="액추에이터 정보를 조회합니다.",
)
async def get_actuator(actuator_id: int) -> dict:
    data = await api_get_actuator(actuator_id=actuator_id)
    return ActuatorResponse.model_validate(data).model_dump()

@mcpServer.tool(
    name="list_sensors",
    description="센서 목록을 조회합니다.",
)
async def list_sensors() -> list[dict]:
    data = await api_get_sensors()
    return [SensorResponse.model_validate(item).model_dump() for item in data]

@mcpServer.tool(
    name="get_sensor",
    description="센서 정보를 조회합니다.",
)
async def get_sensor(sensor_id: int) -> dict:
    data = await api_get_sensor(sensor_id=sensor_id)
    return SensorResponse.model_validate(data).model_dump()

@mcpServer.tool(
    name="set_actuator_state",
    description="액추에이터 상태를 설정합니다.",
)
async def set_actuator_state(
    actuator_id: Annotated[int, Field(description="액추에이터 ID")],
    state: Annotated[int, Field(description="0~level 사이의 값")],
) -> dict:
    await api_update_actuator_state(actuator_id=actuator_id, state=state)
    return {"ok": True}

@mcpServer.tool(
    name="update_device_info",
    description="IoT 기기의 이름(사용자에게 표시되는 이름)과 설명을 업데이트합니다.",
) 
async def update_device_info(
    name: Annotated[str | None, Field(description="기기 이름")] = None,
    description: Annotated[str | None, Field(description="기기 설명")] = None,
) -> dict:
    await api_update_device_info(name=name, description=description)
    return {"ok": True}

@mcpServer.tool(
    name="update_sensor_info",
    description="센서의 이름과 설명을 업데이트합니다.",
)
async def update_sensor_info(
    sensor_id: Annotated[int, Field(description="센서 ID")],
    name: Annotated[str | None, Field(description="센서 이름")] = None,
    description: Annotated[str | None, Field(description="센서 설명")] = None,
) -> dict:
    await api_update_sensor_info(sensor_id=sensor_id, name=name, description=description)
    return {"ok": True}

@mcpServer.tool(
    name="update_actuator_info",
    description="액추에이터의 이름과 설명을 업데이트합니다.",
)
async def update_actuator_info(
    actuator_id: Annotated[int, Field(description="액추에이터 ID")],
    name: Annotated[str | None, Field(description="액추에이터 이름")] = None,
    description: Annotated[str | None, Field(description="액추에이터 설명")] = None,
) -> dict:
    await api_update_actuator_info(actuator_id=actuator_id, name=name, description=description)
    return {"ok": True}