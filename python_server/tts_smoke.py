"""MeloTTS 한국어 합성 스모크 테스트.
서버 없이 모델을 직접 로드해 한국어 WAV를 만들고, 유효성(샘플레이트/길이/피크)을 출력한다.
청취는 사람이 out_kr.wav로 확인.
"""
import time
import numpy as np
import soundfile as sf

from melo_ko_patch import install_eunjeon_shim

install_eunjeon_shim()  # Windows 한국어 G2P 우회 (import melo 이전)

t0 = time.time()
from melo.api import TTS  # noqa: E402

model = TTS(language="KR", device="auto")
load_s = time.time() - t0
spk = model.hps.data.spk2id
sr = model.hps.data.sampling_rate
print(f"loaded in {load_s:.1f}s  spk2id={spk}  sr={sr}")

text = "안녕하세요, 무슨 일이지? 필요한 게 있나?"
t1 = time.time()
audio = model.tts_to_file(text, spk["KR"], output_path=None, speed=1.0)
synth_s = time.time() - t1

audio = np.asarray(audio, dtype=np.float32)
dur = len(audio) / sr
peak = float(np.max(np.abs(audio))) if audio.size else 0.0
print(f"synth in {synth_s:.1f}s  samples={len(audio)}  dur={dur:.2f}s  peak={peak:.3f}")

out = "out_kr.wav"
sf.write(out, audio, sr, format="WAV", subtype="PCM_16")
print(f"wrote {out}  (16-bit PCM mono, {sr} Hz)")

# 유효성 판정: 비-무음 + 텍스트 길이에 걸맞은 재생시간
assert peak > 0.01, "출력이 사실상 무음 — 합성 실패 의심"
assert dur > 0.5, "재생시간이 너무 짧음"
print("SMOKE OK")
