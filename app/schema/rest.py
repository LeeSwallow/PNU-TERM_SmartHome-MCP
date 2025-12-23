from datetime import datetime
from pydantic import BaseModel, Field, model_validator, ConfigDict
from app.model.enums import SensorType

class RestDeviceResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    device_code: str = Field(description="IoT 기기에 내장된 코드")
    name: str | None = Field(default=None, description="IoT 기기 이름")
    description: str | None = Field(default=None, description="IoT 기기 설명")
    created_at: datetime = Field(description="생성 시간")
    updated_at: datetime = Field(description="수정 시간")


class RestActuatorResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int = Field(description="액추에이터 ID", ge=1)
    name: str = Field(description="액추에이터 이름 (unique)")
    name_shown: str | None = Field(default=None, description="표시 이름")
    description: str | None = Field(default=None, description="액추에이터 설명")
    level: int = Field(description="액추에이터 레벨", ge=0)
    state: int | None = Field(default=None, description="액추에이터 상태")
    created_at: datetime = Field(description="생성 시간")
    updated_at: datetime = Field(description="수정 시간")


class RestSensorResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    id: int = Field(description="센서 ID", ge=1)
    name: str = Field(description="센서 이름 (unique)")
    name_shown: str | None = Field(default=None, description="표시 이름")
    sensor_type: SensorType = Field(description="센서 타입")
    state: str | None = Field(default=None, description="센서 상태")
    description: str | None = Field(default=None, description="센서 설명")
    created_at: datetime = Field(description="생성 시간")
    updated_at: datetime = Field(description="수정 시간")
    
class RestActuatorUpdateRequest(BaseModel):
    model_config = ConfigDict(from_attributes=True)
    state: int = Field(description="액추에이터 상태", ge=0)
    