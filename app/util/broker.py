import os
import paho.mqtt.client as mqtt
from dotenv import load_dotenv
from app.util.logging import logging
from app.crud.event.listener import on_register, on_update

load_dotenv()


MQTT_BROKER_URL=os.getenv("MQTT_BROKER_URL")
MQTT_BROKER_PORT=os.getenv("MQTT_BROKER_PORT")
MQTT_USERNAME=os.getenv("MQTT_USERNAME")
MQTT_PASSWORD=os.getenv("MQTT_PASSWORD")

if not MQTT_BROKER_URL or not MQTT_BROKER_PORT or not MQTT_USERNAME or not MQTT_PASSWORD:
    raise ValueError("MQTT 환경 변수가 설정되지 않았습니다.")

MQTT_BROKER_PORT = int(MQTT_BROKER_PORT)

logger = logging.getLogger(__name__)

# 전역 MQTT 클라이언트 (서버 lifespan에서 초기화)
mqttClient: mqtt.Client | None = None


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


def get_mqtt_client() -> mqtt.Client:
    global mqttClient
    client = mqtt.Client()
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.connect_async(MQTT_BROKER_URL, MQTT_BROKER_PORT)

    # 설정된 콜백 함수 등록
    client.on_connect = mqtt_on_connect
    client.message_callback_add("devices/+/register", on_register)
    client.message_callback_add("devices/+/update", on_update)

    mqttClient = client
    return client