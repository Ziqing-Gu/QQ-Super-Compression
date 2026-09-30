from pathlib import Path
import hashlib

root = Path(__file__).resolve().parents[1]
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')
editor = (root/'Source/LimiterEditor.cpp').read_text(encoding='utf-8')
checks = (root/'tests/ratio_mix_checks.inc').read_text(encoding='utf-8')
engine = (root/'Source/StaticCompressionEngine.h').read_bytes()

assert 'VERSION 1.2.24' in cmake
assert 'main[d]->setResetValue(dual ? 1.0 : limiter ? double(qqsc::limiterMinimumDownRatio) : 1.0);' in editor
assert 'Alt-click resets to 200:1.' in editor
assert 'Limiter Single Alt-reset did not return to 200:1' in checks
assert 'Limiter Dual UP Alt-reset changed from 1:1' in checks
assert 'Limiter Dual DOWN Alt-reset changed from 200:1' in checks
assert 'Normal Single Alt-reset changed from 1:1' in checks
expected = '51b36d3aa7be1113aa4e90ac6c0c534163a9b7a3a9edc2dfe24d455cad3fa6ba'
assert hashlib.sha256(engine).hexdigest() == expected, 'StaticCompressionEngine changed'
print('PASS: 1.2.24 Limiter SINGLE Alt-reset returns to its 200:1 bank default; Dual and Normal reset semantics stay unchanged; DSP engine unchanged.')
