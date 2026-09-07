"""UI-only candidate contract. Optional argument: untouched v1.1.5 workspace."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
source = root / "Source"
look = (source / "UTF8LookAndFeel.h").read_text(encoding="utf-8")
material = (source / "WarmKnobAsset.h").read_text(encoding="utf-8")
editor = (source / "PluginEditor.cpp").read_text(encoding="utf-8")
display = (source / "DynamicDisplay.cpp").read_text(encoding="utf-8")
meters = (source / "LevelMeters.cpp").read_text(encoding="utf-8")
assert 'warmKnobs->draw' in look and 'warm::button' in look
assert 'QQSCWarmKnobData::approvedlit_png' in material
assert 'QQSCWarmKnobData::unlitbase_png' in material
assert 'SafePointer<juce::Slider>' in material
assert 'found->position != position' in material
assert 'while (bases.size() >= 4)' in material
assert 'value <= 0.0f' in material
assert 'slider.isEnabled()' in material
assert 'label.isBeingEdited()' in look
assert 'textWhenEditingColourId' in look
assert 'classicTheme ?' in editor
assert 'theme == qqsc::ui::Theme::light ? qqsc::ui::Theme::dark' in editor
assert 'setValue ("uiTheme", qqsc::ui::themeKey (theme))' in editor
assert 'QQSCWarmKnobAssets' in (root / "CMakeLists.txt").read_text(encoding="utf-8")
assert 'knobMaterial()' not in (source / 'WarmMaterial.h').read_text(encoding='utf-8')
assert 'warm::backplate' in editor
assert 'const auto originY = reductionMeter ? fill.getY() : fill.getBottom()' in meters
assert 'const auto leadingY = reductionMeter ? fill.getBottom() : fill.getY()' in meters
assert 'fill.setHeight (fillHeight)' in meters
assert 'tracePaint (qqsc::ui::outputAccent().withAlpha (0.96f))' in display
assert 'dbToY (-45.0f, plot)' in display

if len(sys.argv) > 1:
    baseline = Path(sys.argv[1]) / "Source"
    protected = ["PluginProcessor.cpp", "PluginProcessor.h", "Parameters.h",
                 "StaticCompressionEngine.h", "MeterState.h", "BS1770LoudnessMatch.h",
                 "LevelMeters.h"]
    for name in protected:
        assert (source / name).read_bytes() == (baseline / name).read_bytes(), name
    previous_display = (baseline / "DynamicDisplay.cpp").read_text(encoding="utf-8")
    # Only drawDomainPanel paint is allowed to differ. All history/projection,
    # worker/retry/cache logic before it and paint/legend dispatch after it stay exact.
    paint_start = 'void DynamicDisplay::drawDomainPanel ('
    paint_end = 'void DynamicDisplay::paint ('
    assert display[:display.index(paint_start)] == previous_display[:previous_display.index(paint_start)]
    assert display[display.index(paint_end):] == previous_display[previous_display.index(paint_end):]
    previous = (baseline / "PluginEditor.cpp").read_text(encoding="utf-8")
    layout_marker = 'void QQSuperCompressionAudioProcessorEditor::resized()'
    assert editor[editor.index(layout_marker):] == previous[previous.index(layout_marker):]
    assert (source / "DynamicDisplay.h").read_text(encoding="utf-8").replace(
        '    friend struct QQSCVisualCheck;\n', '') == (baseline / "DynamicDisplay.h").read_text(encoding="utf-8")
    print("PASS: audio/DSP/parameters/history code identical to baseline; complete layout identical.")
print("PASS: warm-only renderer, cached material, continuous live indicators, editable values, remembered theme.")
