import asyncio
from typing import Any, Dict
from collections import defaultdict
from app.util.logging import logging

logger = logging.getLogger(__name__)

# SSE 구독자 레지스트리 (디바이스별)
_subscribers: dict[str, set[asyncio.Queue]] = defaultdict(set)
_event_loop: asyncio.AbstractEventLoop | None = None


def set_event_loop(loop: asyncio.AbstractEventLoop):
    global _event_loop
    _event_loop = loop


def subscribe(device_code: str) -> asyncio.Queue:
    q: asyncio.Queue = asyncio.Queue(maxsize=100)
    _subscribers[device_code].add(q)
    return q


def unsubscribe(device_code: str, q: asyncio.Queue):
    try:
        _subscribers.get(device_code, set()).discard(q)
    except Exception as e:
        logger.error(f"SSE 구독 해제 중 오류: {e}")


def publish_sse_event(device_code: str, event: Dict[str, Any]):
    targets = list(_subscribers.get(device_code, set()))
    if not targets:
        return
    for q in targets:
        try:
            if _event_loop and _event_loop.is_running():
                asyncio.run_coroutine_threadsafe(q.put(event), _event_loop)
            else:
                # 동일 이벤트 루프 컨텍스트라면
                q.put_nowait(event)
        except Exception as e:
            logger.error(f"SSE 이벤트 전달 실패: {e}")