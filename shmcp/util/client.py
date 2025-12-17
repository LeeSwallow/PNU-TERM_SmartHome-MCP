import os
import httpx
from shmcp.schema import DeviceResponse

# mcp.json의 env 설정에서 환경 변수를 가져옴
DEVICE_ID = os.getenv("DEVICE_ID")
SERVER_URL = os.getenv("SERVER_URL")

if not DEVICE_ID:
    raise ValueError("DEVICE_ID 환경 변수가 설정되지 않았습니다.")
if not SERVER_URL:
    raise ValueError("SERVER_URL 환경 변수가 설정되지 않았습니다.")

_client = httpx.AsyncClient(base_url=SERVER_URL, timeout=10.0, follow_redirects=True)

async def get_device_info(device_code: str | None = None) -> DeviceResponse:
    code = device_code or DEVICE_ID
    response = await _client.get(f"/api/v1/devices/{code}")
    response.raise_for_status()
    return DeviceResponse.model_validate(response.json())

async def get_request(path: str) -> dict:
    response = await _client.get(path)
    response.raise_for_status()
    return response.json()

async def post_request(path: str, data: dict) -> dict:
    response = await _client.post(path, json=data)
    response.raise_for_status()
    return response.json()

async def put_request(path: str, data: dict) -> dict:
    response = await _client.put(path, json=data)
    response.raise_for_status()
    return response.json()

# Convenience API wrappers
async def api_get_actuators():
    return await get_request(f"/api/v1/devices/{DEVICE_ID}/actuators/")

async def api_get_actuator(actuator_id: int):
    return await get_request(f"/api/v1/devices/{DEVICE_ID}/actuators/{actuator_id}")

async def api_get_sensors():
    return await get_request(f"/api/v1/devices/{DEVICE_ID}/sensors/")

async def api_get_sensor(sensor_id: int):
    return await get_request(f"/api/v1/devices/{DEVICE_ID}/sensors/{sensor_id}")

async def api_update_actuator_state(actuator_id: int, state: int):
    payload = {"state": state}
    return await post_request(f"/api/v1/devices/{DEVICE_ID}/actuators/{actuator_id}/action", payload)

async def api_update_device_info(name: str | None = None, description: str | None = None):
    payload = {}
    if name is not None:
        payload["name"] = name
    if description is not None:
        payload["description"] = description
    return await put_request(f"/api/v1/devices/{DEVICE_ID}", payload)

async def api_update_sensor_info(sensor_id: int, name: str | None = None, description: str | None = None):
    payload = {}
    if name is not None:
        payload["name"] = name
    if description is not None:
        payload["description"] = description
    return await put_request(f"/api/v1/devices/{DEVICE_ID}/sensors/{sensor_id}", payload)

async def api_update_actuator_info(actuator_id: int, name: str | None = None, description: str | None = None):
    payload = {}
    if name is not None:
        payload["name"] = name
    if description is not None:
        payload["description"] = description
    return await put_request(f"/api/v1/devices/{DEVICE_ID}/actuators/{actuator_id}", payload)