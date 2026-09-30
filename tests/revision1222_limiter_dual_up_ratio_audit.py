from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
limits = (root/'Source/DynamicsLimits.h').read_text(encoding='utf-8')
params = (root/'Source/Parameters.h').read_text(encoding='utf-8')
proc = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
editor = (root/'Source/LimiterEditor.cpp').read_text(encoding='utf-8')
main_editor = (root/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')
engine = (root/'Source/StaticCompressionEngine.h').read_bytes()

assert 'VERSION 1.2.23' in cmake
assert 'limiterDualMinimumUpRatio = 1.0f / 8.0f' in limits
assert 'limiter ? limiterDualMinimumUpRatio : minimumUpRatio' in params
assert 'group==1 ? qqsc::params::dynamicsRatioRange(qqsc::limiterDualMinimumUpRatio,1.0f)' in proc
assert 'limiter ? qqsc::limiterDualMinimumUpRatio : qqsc::minimumUpRatio' in editor
assert 'Upward Ratio: 1:8 to 1:1' in editor
assert 'processor.isLimiterMode() ? "Upward Ratio: 1:8 to 1:1"' in main_editor

# The compression transfer engine itself must not be edited by this feature.
import hashlib
expected = '51b36d3aa7be1113aa4e90ac6c0c534163a9b7a3a9edc2dfe24d455cad3fa6ba'
assert hashlib.sha256(engine).hexdigest() == expected, 'StaticCompressionEngine changed'

print('PASS: 1.2.22 Limiter Dual UP 1:8..1:1 contract remains present in the 1.2.23 candidate; StaticCompressionEngine remains unchanged.')
