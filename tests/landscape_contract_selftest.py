"""UI-only 3:2 contract; optional active v1.1.8 source parity check."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
editor = (root / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
for fragment in ['defaultEditorWidth = 1200', 'defaultEditorHeight = 800',
                 'minEditorHeight = 672', 'maxEditorHeight = 1200',
                 'setFixedAspectRatio (editorAspectRatio)',
                 'getIntValue ("landscapeEditorWidth", defaultEditorWidth)',
                 'setValue ("landscapeEditorWidth", getWidth())',
                 'AffineTransform::scale (uiScale)', 'removeFromTop (530)']:
    assert fragment in editor, fragment
assert 'setValue ("editorWidth"' not in editor
assert 'setValue ("editorHeight"' not in editor
if len(sys.argv) > 1:
    previous = Path(sys.argv[1])
    protected = list((root / 'Source').glob('*')) + list((root / 'Assets').rglob('*'))
    for path in protected:
        if path.is_file() and path.name != 'PluginEditor.cpp':
            assert path.read_bytes() == (previous / path.relative_to(root)).read_bytes(), path
    old = (previous / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
    begin = 'std::unique_ptr<juce::PropertiesFile>'
    end = 'void QQSuperCompressionAudioProcessorEditor::paint ('
    assert editor[editor.index(begin):editor.index(end)] == old[old.index(begin):old.index(end)]
    print('PASS: all DSP, parameters, state, Display/meter code, theme renderers and assets byte-identical to active 1.1.8.')
print('PASS: 3:2 uniform scaling, bounded size, independent persistent size and retained legacy preferences.')
