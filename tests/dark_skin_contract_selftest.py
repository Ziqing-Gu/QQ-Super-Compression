"""1.1.8 UI-only contract; compare against active (not frozen) 1.1.7 source."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
source = root / 'Source'
read = lambda name: (source / name).read_text(encoding='utf-8')
look = read('UTF8LookAndFeel.h')
editor = read('PluginEditor.cpp')
display = read('DynamicDisplay.cpp')
dark = read('RefinedDarkKnob.h')
assert 'enum class Theme { light, dark, classic }' in look
assert 'properties.getBoolValue ("classicTheme", false) ? Theme::classic : Theme::light' in look
assert 'uiProperties->setValue ("uiTheme", qqsc::ui::themeKey (theme))' in editor
assert 'darkKnobs->draw' in look and 'warmKnobs->draw' in look
assert 'if (value > 0.0f)' in dark and 'SafePointer<juce::Slider>' in dark
assert 'entry->value != value' in dark and 'while (bases.size() >= 4)' in dark
assert 'dark_refined::Renderer' in look
assert 'darkBottomPanel.draw' in editor
panel = read('DarkPanelMaterial.h')
assert 'if (! finish.isValid()) generate();' in panel
assert 'std::uint32_t' in panel and 'getGenerationCount()' in panel
assert '0xff1c1d1f' in display  # solid near-black plot; no texture generation
assert '0xff191c1e' in read('LevelMeters.cpp')

if len(sys.argv) > 1:
    baseline = Path(sys.argv[1])
    for name in ['PluginProcessor.cpp', 'PluginProcessor.h', 'Parameters.h',
                 'StaticCompressionEngine.h', 'MeterState.h', 'BS1770LoudnessMatch.h',
                 'DynamicDisplay.h', 'DynamicDisplay.cpp', 'LevelMeters.h', 'LevelMeters.cpp',
                 'WarmMaterial.h']:
        assert (source / name).read_bytes() == (baseline / 'Source' / name).read_bytes(), name
    for name in ['approved-lit.png', 'unlit-base.png']:
        assert (root / 'Assets' / 'WarmKnob' / name).read_bytes() == (baseline / 'Assets' / 'WarmKnob' / name).read_bytes(), name
    # User subsequently requested a longer Light pointer; retain every other
    # part of its image compositor, embedded material and cache (ignore EOF whitespace).
    pointer_start = '    static void paintPointer('
    pointer_end = '\n};\n'
    def without_pointer(text):
        begin = text.index(pointer_start)
        end = text.index(pointer_end, begin)
        return (text[:begin] + text[end:]).rstrip()
    assert without_pointer(read('WarmKnobAsset.h')) == without_pointer((baseline / 'Source' / 'WarmKnobAsset.h').read_text(encoding='utf-8'))
    prior_display = (baseline / 'Source' / 'DynamicDisplay.cpp').read_text(encoding='utf-8')
    start, end = 'void DynamicDisplay::drawDomainPanel (', 'void DynamicDisplay::paint ('
    assert display[:display.index(start)] == prior_display[:prior_display.index(start)]
    assert display[display.index(end):] == prior_display[prior_display.index(end):]
    prior_editor = (baseline / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')
    marker = 'void QQSuperCompressionAudioProcessorEditor::resized()'
    assert editor[editor.index(marker):] == prior_editor[prior_editor.index(marker):]
    # Exactly preserve both established chassis branches after the new early Dark return.
    marker = '    if (! qqsc::ui::isClassicTheme())'
    old_paint = prior_editor[prior_editor.index('void QQSuperCompressionAudioProcessorEditor::paint ('):]
    new_paint = editor[editor.index('void QQSuperCompressionAudioProcessorEditor::paint ('):]
    assert old_paint[old_paint.index(marker):] == new_paint[new_paint.index(marker):]
    print('PASS: DSP/state/parameters/history and LIGHT assets identical; layout and original chassis branches identical.')
print('PASS: retained three themes/migration, pure Display, cached bottom grain and approved blue-light renderer.')
