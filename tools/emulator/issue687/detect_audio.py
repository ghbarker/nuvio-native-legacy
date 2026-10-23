"""Independent speech-language detection of PCM decoded by Android Media3."""
import json,sys,wave
import numpy as np
from pathlib import Path
from faster_whisper import WhisperModel
out=Path(sys.argv[1] if len(sys.argv)>1 else 'emulator-results');model=WhisperModel('tiny',device='cpu',compute_type='int8',cpu_threads=2)
summary=json.loads((out/'summary.json').read_text());results=[]
for case in summary['completed_cases']:
 path=out/case['scenario']/'decoded-audio.wav'
 # The evidence is known 16-kHz mono PCM WAV. Decode it directly to avoid
 # a faster-whisper/PyAV keyword incompatibility in the hosted environment.
 with wave.open(str(path),'rb') as wav:
  assert wav.getframerate()==16000 and wav.getnchannels()==1 and wav.getsampwidth()==2
  audio=np.frombuffer(wav.readframes(wav.getnframes()),dtype=np.int16).astype(np.float32)/32768.0
 segments,info=model.transcribe(audio,beam_size=1,vad_filter=True,condition_on_previous_text=False)
 count=sum(1 for _ in segments)
 r={'scenario':case['scenario'],'expected_language':case['expected_language'],'detected_language':info.language,'probability':info.language_probability,'speech_segments':count}
 r['matches']=info.language==case['expected_language']
 results.append(r)
 print('SPEECH '+json.dumps(r),flush=True)
 (out/case['scenario']/'speech-language.json').write_text(json.dumps(r,indent=2)+'\n')
summary['speech_language_detection']=results
(out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
# Language recognition is evidence, separate from successful Media3 playback.
# Do not turn uncertain/short musical snippets into a false player failure.
assert all(r['matches'] for r in results if r['probability']>=0.7 and r['speech_segments']>0),results
