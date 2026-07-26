"""Windows에서 MeloTTS 한국어 G2P(g2pkk) 의존성 우회.

g2pkk은 Windows에서 `eunjeon`(한국어 MeCab)을 요구하지만
- eunjeon 은 Windows에서 네이티브 빌드가 실패하고,
- python-mecab-ko(모듈 `mecab`)는 MeloTTS가 끌어오는 mecab-python3(모듈 `MeCab`)와
  대소문자 무시 파일시스템에서 폴더명이 충돌한다.
대신 wheel로 깔끔히 설치되는 kiwipiepy를 eunjeon.Mecab 인터페이스(.pos)로 감싸
sys.modules 에 주입한다. 반드시 `import melo` 보다 먼저 호출해야 한다.

g2pkk의 mecab 사용처는 발음 규칙 일부(의/ㄹ 종성/의존명사)뿐이라, kiwi 태그셋이
mecab-ko와 완전히 같지 않아도 합성은 정상 동작한다(불일치 시 해당 규칙만 생략).
"""

import sys
import types
from importlib.machinery import ModuleSpec


def install_eunjeon_shim() -> None:
    if "eunjeon" in sys.modules:
        return

    from kiwipiepy import Kiwi

    class Mecab:
        """g2pkk이 기대하는 eunjeon.Mecab 대체 (.pos만 제공)."""

        def __init__(self, *args, **kwargs) -> None:
            self._kiwi = Kiwi()

        def pos(self, text):
            return [(tok.form, tok.tag) for tok in self._kiwi.tokenize(text)]

    shim = types.ModuleType("eunjeon")
    shim.Mecab = Mecab
    shim.__spec__ = ModuleSpec("eunjeon", loader=None)
    sys.modules["eunjeon"] = shim
