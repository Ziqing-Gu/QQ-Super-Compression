from pathlib import Path
import hashlib

root = Path(__file__).resolve().parents[1]
limits = (root/'Source/DynamicsLimits.h').read_text(encoding='utf-8')
params = (root/'Source/Parameters.h').read_text(encoding='utf-8')
proc = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
editor = (root/'Source/LimiterEditor.cpp').read_text(encoding='utf-8')
main_editor = (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
checks = (root/'tests/ratio_mix_checks.inc').read_text(encoding='utf-8')
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')
engine = (root/'Source/StaticCompressionEngine.h').read_bytes()

assert 'VERSION 1.2.23' in cmake or 'VERSION 1.2.24' in cmake
assert 'limiterMinimumUpRatio = 1.0f / 8.0f' in limits
assert 'limiterDualMinimumUpRatio = 1.0f / 8.0f' in limits
assert 'return { T(limiterMinimumUpRatio), T(maximumDownRatio)' in params
assert 'limiter ? limiterMinimumUpRatio : minimumUpRatio' in params
assert 'limiter ? limiterDualMinimumUpRatio : minimumUpRatio' in params
assert 'group==0 ? qqsc::params::limiterSingleRange<float>()' in proc
assert 'group==1 ? qqsc::params::dynamicsRatioRange(qqsc::limiterDualMinimumUpRatio,1.0f)' in proc
assert 'Up Ratio 1:8-1:1' in editor
assert 'Up 1:8 to 1:1; Down 200:1 to 1000:1' in editor
assert 'processor.isLimiterMode() ? "Up 1:8 to 1:1; Down 200:1 to 1000:1' in main_editor
assert 'range(qqsc::params::limiterRatioIds[d],.125f,1000);' in checks
assert 'range(qqsc::params::limiterUpRatioIds[d],.125f,1);' in checks
assert 'Limiter Single numeric entry escaped 1:8 floor' in checks
assert 'Normal Single range changed' in checks

expected = '51b36d3aa7be1113aa4e90ac6c0c534163a9b7a3a9edc2dfe24d455cad3fa6ba'
assert hashlib.sha256(engine).hexdigest() == expected, 'StaticCompressionEngine changed'

print('PASS: 1.2.23+ Limiter Single UP and Dual UP are both 1:8..1:1; Normal ranges remain wide; StaticCompressionEngine unchanged.')
