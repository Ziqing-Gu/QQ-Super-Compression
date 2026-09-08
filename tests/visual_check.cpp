#include "PluginEditor.h"
#include <iostream>
#include <fstream>

struct QQSCVisualCheck
{
    static std::unique_ptr<juce::FileOutputStream> output (const juce::File& file)
    {
        auto stream = file.createOutputStream();
        if (stream == nullptr || ! stream->setPosition (0) || stream->truncate().failed())
            throw std::runtime_error ("Cannot create test output");
        return stream;
    }
    static void parameter (QQSuperCompressionAudioProcessor& p, const char* id, float value)
    {
        auto* parameter = p.getAPVTS().getParameter (id);
        if (parameter == nullptr) throw std::runtime_error (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    static void snapshot (QQSuperCompressionAudioProcessorEditor& editor, const juce::File& file, float scale = 1.0f)
    {
        if (editor.getWidth() * 2 != editor.getHeight() * 3
            || editor.getConstrainer()->getFixedAspectRatio() != 1.5)
            throw std::runtime_error ("Landscape aspect ratio changed");
        for (auto* child : editor.contentRoot.getChildren())
            if (child->isVisible() && ! editor.contentRoot.getLocalBounds().contains (child->getBounds()))
                throw std::runtime_error ("Visible control exceeds landscape bounds");
        if (editor.display.getBounds() != juce::Rectangle<int> (22, 80, 866, 530)
            || editor.meters.getBounds() != juce::Rectangle<int> (978, 80, 200, 530))
            throw std::runtime_error ("Landscape Display/meter geometry changed");
        const auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, scale);
        auto stream = output (file);
        if (stream == nullptr || ! juce::PNGImageFormat().writeImageToStream (image, *stream))
            throw std::runtime_error ("PNG write failed");
        if (editor.theme == qqsc::ui::Theme::light)
        {
            auto regions = output (file.getSiblingFile (file.getFileNameWithoutExtension() + "-rotary-bounds.txt"));
            std::function<void(juce::Component&)> visit = [&] (juce::Component& c)
            {
                if (auto* slider = dynamic_cast<juce::Slider*> (&c))
                {
                    const auto style = slider->getSliderStyle();
                    if (style >= juce::Slider::Rotary && style <= juce::Slider::RotaryHorizontalVerticalDrag)
                    {
                        const auto r = editor.getLocalArea (slider, slider->getLookAndFeel().getSliderLayout (*slider).sliderBounds);
                        regions->writeText (r.toString() + "\n", false, false, "\n");
                    }
                }
                for (auto* child : c.getChildren()) if (child->isVisible()) visit (*child);
            };
            visit (editor);
            if (file.getFileNameWithoutExtension() == "warm-ST")
                for (float pixelScale : {1.0f, 2.0f})
                {
                    const auto crop = editor.createComponentSnapshot ({16, 620, 1168, 162}, true, pixelScale);
                    auto cropStream = output (file.getSiblingFile (pixelScale == 1.0f ? "warm-bottom.png" : "warm-bottom-2x.png"));
                    if (! juce::PNGImageFormat().writeImageToStream (crop, *cropStream))
                        throw std::runtime_error ("Light bottom snapshot failed");
                }
        }
    }

    static void bounds (juce::Component& c, juce::OutputStream& out, const juce::String& path)
    {
        juce::String description = path + "|" + c.getBounds().toString() + "|" + juce::String (c.isVisible() ? 1 : 0);
        if (auto* button = dynamic_cast<juce::TextButton*> (&c)) description += "|" + button->getButtonText();
        out.writeText (description + "\n", false, false, "\n");
        for (int i = 0; i < c.getNumChildComponents(); ++i)
            bounds (*c.getChildComponent (i), out, path + "/" + juce::String (i));
    }

    static void meterGradientCheck (QQSuperCompressionAudioProcessorEditor& editor,
                                    QQSuperCompressionAudioProcessor& processor, const juce::File& dir)
    {
        auto& meters = processor.getMeterState();
        constexpr float level = 0.65f;
        meters.inputDb0.store (-60.0f + 63.0f * level);
        meters.inputDb1.store (-60.0f + 63.0f * level);
        meters.outputDb0.store (-60.0f + 63.0f * level);
        meters.outputDb1.store (-60.0f + 63.0f * level);
        meters.gainReductionDb0.store (36.0f * level);
        meters.gainReductionDb1.store (36.0f * level);
        meters.gainReductionHoldDb0.store (0.0f);
        meters.gainReductionHoldDb1.store (0.0f);
        const auto image = editor.meters.createComponentSnapshot (editor.meters.getLocalBounds());
        auto stream = output (dir.getChildFile ("warm-meter-depth.png"));
        if (! juce::PNGImageFormat().writeImageToStream (image, *stream))
            throw std::runtime_error ("Meter PNG write failed");
        const float groupWidth = (image.getWidth() - 20.0f) / 3.0f;
        const float firstBarCentre = 6.0f + (groupWidth - 2.5f) * 0.25f;
        const float barTop = 6.0f + 24.0f + 18.0f + 3.0f;
        const float barBottom = image.getHeight() - 6.0f - 32.0f - 3.0f;
        const float fillHeight = (barBottom - barTop) * level;
        const auto luminance = [&] (int x, float y)
        {
            float total = 0.0f;
            for (int dy = -5; dy <= 5; ++dy)
            {
                const auto pixel = image.getPixelAt (x, juce::roundToInt (y) + dy);
                total += pixel.getFloatRed() * 0.2126f + pixel.getFloatGreen() * 0.7152f
                         + pixel.getFloatBlue() * 0.0722f;
            }
            return total / 11.0f;
        };
        for (int group = 0; group < 3; ++group)
        {
            const auto x = juce::roundToInt (firstBarCentre + group * (groupWidth + 4.0f));
            const auto richY = group == 2 ? barTop + fillHeight * 0.10f : barBottom - fillHeight * 0.10f;
            const auto paleY = group == 2 ? barTop + fillHeight * 0.90f : barBottom - fillHeight * 0.90f;
            if (luminance (x, paleY) - luminance (x, richY) < 0.12f)
                throw std::runtime_error ("Meter vertical tonal contrast missing");
        }
        std::cout << "PASS: actual gray/orange/cyan meters have vertical depth; GR lighting is mirrored.\n";
    }

    static void darkCheck (QQSuperCompressionAudioProcessorEditor& editor,
                            QQSuperCompressionAudioProcessor& processor, const juce::File& dir)
    {
        editor.theme = qqsc::ui::Theme::dark;
        editor.applyTheme();
        snapshot (editor, dir.getChildFile ("dark-cache-prime.png"));
        auto& renderer = *editor.utf8LookAndFeel.darkKnobs;
        juce::Slider control;
        juce::Image buffer (juce::Image::ARGB, 160, 160, true);
        juce::Graphics g (buffer);
        renderer.draw (g, { 0, 0, 100, 100 }, 0.5f, control);
        const auto count = renderer.getRenderCount();
        for (int i = 0; i < 200; ++i) renderer.draw (g, { 0, 0, 100, 100 }, 0.5f, control);
        if (renderer.getRenderCount() != count) throw std::runtime_error ("Dark cache missed unchanged knob");
        renderer.draw (g, { 0, 0, 100, 100 }, 0.6f, control);
        if (renderer.getRenderCount() != count + 1) throw std::runtime_error ("Dark cache failed to refresh");

        qqsc::dark_refined::Material material;
        const auto zero = material.render (0.0f, 400);
        for (float value : { 0.1f, 0.5f, 1.0f })
        {
            const auto frame = material.render (value, 400);
            if (frame.getPixelAt (200, 180) != zero.getPixelAt (200, 180)
                || frame.getPixelAt (200, 357) != zero.getPixelAt (200, 357))
                throw std::runtime_error ("Dark fixed body/shadow changed");
            for (float fraction : { 0.04f, 0.20f, 0.40f, 0.65f, 0.90f })
            {
                const auto angle = qqsc::dark_refined::Material::angleForValue (fraction);
                const auto x = juce::roundToInt ((50.0f + 33.7f * std::sin (angle)) * 4.0f);
                const auto y = juce::roundToInt ((46.0f - 33.7f * std::cos (angle)) * 4.0f);
                const auto a = zero.getPixelAt (x, y);
                const auto b = frame.getPixelAt (x, y);
                const int blueIncrease = b.getBlue() - a.getBlue();
                if (fraction < value && (blueIncrease < 35 || b.getBlue() - b.getRed() < 12))
                    throw std::runtime_error ("Dark active arc is not blue and lit");
                if (fraction > value && std::abs (blueIncrease) > 3)
                    throw std::runtime_error ("Dark inactive arc emits light");
            }
        }
        std::vector<double> timings;
        material.render (0.0f, 160);
        for (int i = 0; i < 40; ++i)
        {
            const auto begin = juce::Time::getMillisecondCounterHiRes();
            material.render (i / 39.0f, 160);
            timings.push_back (juce::Time::getMillisecondCounterHiRes() - begin);
        }
        std::sort (timings.begin(), timings.end());
        std::cout << "PASS: Dark zero/progressive blue light, fixed metal/shadow, 200 cached repaints; 160px median "
                  << timings[20] << " ms, p95 " << timings[38] << " ms.\n";
        juce::Image details (juce::Image::RGB, 1020, 400, true);
        {
            juce::Graphics graphics (details);
            graphics.fillAll (qqsc::ui::canvas());
            const float positions[] { 0.0f, 0.1f, 0.5f, 0.75f, 1.0f };
            for (int i = 0; i < 5; ++i)
            {
                editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 2, 4, 200, 200,
                    positions[i], qqsc::dark::Material::start, qqsc::dark::Material::end, control);
                graphics.setColour (qqsc::ui::text());
                graphics.setFont (16.0f);
                graphics.drawText (juce::String (juce::roundToInt (positions[i] * 100.0f)) + "%",
                                   i * 204, 204, 204, 24, juce::Justification::centred);
                editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 62, 252, 80, 90,
                    positions[i], qqsc::dark::Material::start, qqsc::dark::Material::end, control);
            }
        }
        auto detailStream = output (dir.getChildFile ("dark-control-detail.png"));
        if (! juce::PNGImageFormat().writeImageToStream (details, *detailStream))
            throw std::runtime_error ("Dark detail PNG failed");

        // Production bottom finish is cached and independent of parameter motion.
        juce::Image panelImage (juce::Image::RGB, 988, 162, true);
        const auto panelCount = editor.darkBottomPanel.getGenerationCount();
        for (int i = 0; i < 200; ++i)
        {
            juce::Graphics panelGraphics (panelImage);
            panelGraphics.fillAll (qqsc::ui::canvas());
            editor.darkBottomPanel.draw (panelGraphics, {0, 0, 988, 162}, 15.0f);
        }
        if (editor.darkBottomPanel.getGenerationCount() != panelCount || panelCount != 1)
            throw std::runtime_error ("Bottom panel finish was regenerated on repaint");
        qqsc::dark::BottomPanelMaterial independentPanel;
        juce::Image secondPanel (juce::Image::RGB, 988, 162, true);
        {
            juce::Graphics panelGraphics (secondPanel);
            panelGraphics.fillAll (qqsc::ui::canvas());
            independentPanel.draw (panelGraphics, {0, 0, 988, 162}, 15.0f);
        }
        for (int y = 0; y < 162; ++y)
            for (int x = 0; x < 988; ++x)
                if (panelImage.getPixelAt (x, y) != secondPanel.getPixelAt (x, y))
                    throw std::runtime_error ("Bottom grain is not deterministic");
        auto panelStream = output (dir.getChildFile ("dark-panel-material.png"));
        if (! juce::PNGImageFormat().writeImageToStream (panelImage, *panelStream))
            throw std::runtime_error ("Panel finish PNG failed");
        for (float pixelScale : {1.0f, 2.0f})
        {
            const auto crop = editor.createComponentSnapshot ({16, 620, 1168, 162}, true, pixelScale);
            auto stream = output (dir.getChildFile (pixelScale == 1.0f ? "dark-bottom.png" : "dark-bottom-2x.png"));
            if (! juce::PNGImageFormat().writeImageToStream (crop, *stream))
                throw std::runtime_error ("Actual bottom panel PNG failed");
        }
        std::cout << "PASS: stationary deterministic bottom grain, one generation across 200 paints.\n";

        auto& m = processor.getMeterState();
        m.inputDb0.store (-18.1f); m.inputDb1.store (-18.3f);
        m.outputDb0.store (-17.2f); m.outputDb1.store (-17.4f);
        m.gainReductionDb0.store (1.6f); m.gainReductionDb1.store (1.6f);
        const auto meters = editor.meters.createComponentSnapshot (editor.meters.getLocalBounds());
        const auto meanLuma = [&] (int x, int y)
        {
            float total = 0.0f;
            for (int dy = -6; dy <= 6; ++dy)
            {
                const auto c = meters.getPixelAt (x, y + dy);
                total += c.getFloatRed() * 0.2126f + c.getFloatGreen() * 0.7152f + c.getFloatBlue() * 0.0722f;
            }
            return total / 13.0f;
        };
        const int firstBarX = juce::roundToInt (6.0f + ((meters.getWidth() - 20.0f) / 3.0f - 2.5f) * 0.25f);
        if (meanLuma (firstBarX, meters.getHeight() - 100) - meanLuma (firstBarX, 100) < 0.18f)
            throw std::runtime_error ("Input active/inactive contrast too low");
        snapshot (editor, dir.getChildFile ("dark-meter-reference.png"));
        const auto audioState = processor.getAPVTS().copyState();
        juce::PropertiesFile::Options options;
        options.millisecondsBeforeSaving = -1;
        const auto prefFile = dir.getChildFile ("test-ui-preference.xml");
        editor.uiProperties = std::make_unique<juce::PropertiesFile> (prefFile, options);
        editor.uiProperties->setValue ("classicTheme", true);
        editor.uiProperties->removeValue ("uiTheme");
        if (qqsc::ui::themeFromPreferences (*editor.uiProperties) != qqsc::ui::Theme::classic)
            throw std::runtime_error ("Legacy Classic preference did not migrate to Classic");
        editor.uiProperties->setValue ("classicTheme", false);
        if (qqsc::ui::themeFromPreferences (*editor.uiProperties) != qqsc::ui::Theme::light)
            throw std::runtime_error ("Legacy Light preference migration failed");
        for (auto expected : {qqsc::ui::Theme::classic, qqsc::ui::Theme::light, qqsc::ui::Theme::dark})
        {
            editor.toggleTheme();
            juce::PropertiesFile reopened (prefFile, options);
            if (editor.theme != expected || qqsc::ui::themeFromPreferences (reopened) != expected
                || editor.themeButton.getButtonText() != qqsc::ui::themeLabel (expected))
                throw std::runtime_error ("Three-theme cycle or disk preference restore failed");
        }
        editor.uiProperties.reset();
        if (! audioState.isEquivalentTo (processor.getAPVTS().copyState()))
            throw std::runtime_error ("Theme changed audio parameter state");
        for (int i = 0; i < editor.ratioSlider.getNumChildComponents(); ++i)
            if (auto* label = dynamic_cast<juce::Label*> (editor.ratioSlider.getChildComponent (i)))
            {
                label->showEditor();
                snapshot (editor, dir.getChildFile ("dark-numeric-edit.png"));
                label->hideEditor (true);
                break;
            }
        editor.setSize (1008, 672);
        snapshot (editor, dir.getChildFile ("dark-minimum.png"));
        editor.setSize (1800, 1200);
        snapshot (editor, dir.getChildFile ("dark-maximum.png"));
        editor.setSize (1200, 800);
        std::cout << "PASS: Dark Input active/inactive contrast, theme toggle/unchanged APVTS, numeric entry and sizes.\n";
    }

    static int run (const juce::File& dir)
    {
        dir.createDirectory();
        QQSuperCompressionAudioProcessor processor;
        processor.setRateAndBufferSizeDetails (48000.0, 800);
        processor.prepareToPlay (48000.0, 800);
        QQSuperCompressionAudioProcessorEditor editor (processor);
        // Never change the user's remembered size or theme while rendering tests.
        editor.uiProperties.reset();
        editor.stopTimer();
        editor.display.stopTimer();
        editor.setSize (1200, 800);
        parameter (processor, "ratio", 5.0f);
        parameter (processor, "thresholdDb", -28.0f);
        parameter (processor, "makeupGainDb", 3.4f);
        parameter (processor, "mix", 76.0f);
        parameter (processor, "lookaheadMs", 26.0f);
        juce::AudioBuffer<float> audio (2, 800);
        juce::MidiBuffer midi;
        const auto feedHistory = [&]
        {
        for (int frame = 0; frame < 500; ++frame)
        {
            for (int i = 0; i < 800; ++i)
            {
                const double t = static_cast<double> (frame * 800 + i) / 48000.0;
                const double pulse = std::pow (0.5 + 0.5 * std::sin (t * 6.4), 2.5);
                const double phrase = 0.4 + 0.6 * std::pow (0.5 + 0.5 * std::sin (t * 2.3 + 0.7), 1.7);
                const float envelope = static_cast<float> ((0.035 + 0.21 * pulse) * phrase);
                audio.setSample (0, i, envelope * static_cast<float> (std::sin (t * 1137.0) + 0.3 * std::sin (t * 2763.0)));
                audio.setSample (1, i, envelope * static_cast<float> (std::sin (t * 1137.0 + 0.25) + 0.26 * std::sin (t * 2701.0)));
            }
            processor.processBlock (audio, midi);
            editor.display.timerCallback();
        }
        };
        feedHistory();
        editor.timerCallback();
        // Validate production cache and material, not the separate prototype.
        editor.theme = qqsc::ui::Theme::light;
        editor.applyTheme();
        snapshot (editor, dir.getChildFile ("warm-cache-prime.png"));
        {
            auto& renderer = *editor.utf8LookAndFeel.warmKnobs;
            juce::Image buffer (juce::Image::ARGB, 100, 100, true);
            juce::Graphics g (buffer);
            juce::Slider control;
            renderer.draw (g, { 0, 0, 90, 90 }, 0.5f, control);
            const auto before = renderer.getRenderCount();
            for (int i = 0; i < 200; ++i) renderer.draw (g, { 0, 0, 90, 90 }, 0.5f, control);
            if (renderer.getRenderCount() != before) throw std::runtime_error ("Unchanged knobs recomposed");
            renderer.draw (g, { 0, 0, 90, 90 }, 0.6f, control);
            if (renderer.getRenderCount() != before + 1) throw std::runtime_error ("Changed knob not refreshed");
            std::cout << "PASS: 200 unchanged production knob paints use cached frames; parameter changes refresh once.\n";
        }
        {
            qqsc::warm_asset::Material material;
            const auto zero = material.render (0.0f, 400);
            const auto half = material.render (0.5f, 400);
            const auto full = material.render (1.0f, 400);
            if (zero.getPixelAt (200, 150) != full.getPixelAt (200, 150)
                || zero.getPixelAt (200, 346) != full.getPixelAt (200, 346))
                throw std::runtime_error ("Fixed material/shadow changed");
            for (int i = 0; i <= 1000; ++i)
                if (qqsc::warm_asset::Material::emission (0.0f, i / 1000.0f, 0.03f) != 0.0f)
                    throw std::runtime_error ("Zero emits light");
            if (qqsc::ui::grAccent() != qqsc::ui::cyanAccent())
                throw std::runtime_error ("Light GR palette mismatch");
            std::vector<double> timings;
            material.render (0.0f, 160);
            for (int i = 0; i < 40; ++i)
            {
                const auto begin = juce::Time::getMillisecondCounterHiRes();
                material.render (i / 39.0f, 160);
                timings.push_back (juce::Time::getMillisecondCounterHiRes() - begin);
            }
            std::sort (timings.begin(), timings.end());
            std::cout << "PASS: production zero-light/fixed material checks; 160px composition median "
                      << timings[20] << " ms, p95 " << timings[38] << " ms.\n";
        }
        for (auto skinTheme : {qqsc::ui::Theme::light, qqsc::ui::Theme::dark, qqsc::ui::Theme::classic})
        {
            editor.theme = skinTheme;
            editor.applyTheme();
            const bool dark = skinTheme == qqsc::ui::Theme::dark;
            const juce::String skin = dark ? "dark" : (skinTheme == qqsc::ui::Theme::classic ? "classic" : "warm");
            if (dark && (qqsc::ui::grAccent() != qqsc::ui::cyanAccent()
                            || editor.themeButton.getButtonText() != "DARK"))
                throw std::runtime_error ("Dark palette or theme label mismatch");
            snapshot (editor, dir.getChildFile (skin + "-ST.png"));
            snapshot (editor, dir.getChildFile (skin + "-ST-2x.png"), 2.0f);
            auto layout = output (dir.getChildFile (skin + "-ST-bounds.txt"));
            bounds (editor.contentRoot, *layout, "root");
            editor.toggleSidechainPanel();
            snapshot (editor, dir.getChildFile (skin + "-sidechain.png"));
            editor.toggleSidechainPanel();
        }
        editor.theme = qqsc::ui::Theme::light;
        editor.applyTheme();
        for (int mode : {qqsc::params::midSide, qqsc::params::leftRight})
        {
            parameter (processor, "processingMode", static_cast<float> (mode));
            editor.timerCallback();
            feedHistory();
            const juce::String modeName = mode == qqsc::params::midSide ? "MS" : "LR";
            snapshot (editor, dir.getChildFile ("warm-" + modeName + ".png"));
            auto layout = output (dir.getChildFile ("warm-" + modeName + "-bounds.txt"));
            bounds (editor.contentRoot, *layout, "root");
            for (auto skinTheme : {qqsc::ui::Theme::dark, qqsc::ui::Theme::classic})
            {
                editor.theme = skinTheme;
                editor.applyTheme();
                const juce::String name = skinTheme == qqsc::ui::Theme::dark ? "dark" : "classic";
                snapshot (editor, dir.getChildFile (name + "-" + modeName + ".png"));
                auto skinBounds = output (dir.getChildFile (name + "-" + modeName + "-bounds.txt"));
                bounds (editor.contentRoot, *skinBounds, "root");
            }
            editor.theme = qqsc::ui::Theme::light;
            editor.applyTheme();
        }
        parameter (processor, "processingMode", static_cast<float> (qqsc::params::stereoLinked));
        editor.timerCallback();
        feedHistory();
        editor.setSize (1008, 672);
        snapshot (editor, dir.getChildFile ("warm-minimum.png"));
        editor.setSize (1800, 1200);
        snapshot (editor, dir.getChildFile ("warm-maximum.png"));
        editor.setSize (1200, 800);
        for (int i = 0; i < editor.ratioSlider.getNumChildComponents(); ++i)
            if (auto* label = dynamic_cast<juce::Label*> (editor.ratioSlider.getChildComponent (i)))
            {
                label->showEditor();
                snapshot (editor, dir.getChildFile ("warm-numeric-edit.png"));
                label->hideEditor (true);
                break;
            }
        // Render the same production LookAndFeel at compact and large knob sizes.
        juce::Image details (juce::Image::RGB, 1020, 400, true);
        {
        juce::Graphics graphics (details);
        graphics.fillAll (qqsc::ui::canvas());
        juce::Slider dial;
        dial.setLookAndFeel (&editor.utf8LookAndFeel);
        dial.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
        for (int i = 0; i < 5; ++i)
        {
            const float positions[] { 0.0f, 0.1f, 0.5f, 0.75f, 1.0f };
            const float position = positions[i];
            editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 2, 4, 200, 200,
                position, juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, dial);
            graphics.setColour (qqsc::ui::text());
            graphics.setFont (16.0f);
            graphics.drawText (juce::String (juce::roundToInt (position * 100.0f)) + "%", i * 204, 204, 204, 24, juce::Justification::centred);
            editor.utf8LookAndFeel.drawRotarySlider (graphics, i * 204 + 62, 252, 80, 90,
                position, juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, dial);
        }
        dial.setLookAndFeel (nullptr);
        } // Flush deferred native drawing before PNG encoding.
        auto detailStream = output (dir.getChildFile ("warm-control-detail.png"));
        if (! juce::PNGImageFormat().writeImageToStream (details, *detailStream)) return 3;
        meterGradientCheck (editor, processor, dir);
        darkCheck (editor, processor, dir);
        processor.releaseResources();
        std::cout << "PASS: actual editor offscreen snapshots; user preferences not written.\n";
        return 0;
    }
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    if (argc != 2) return 2;
    try { return QQSCVisualCheck::run (juce::File (juce::String::fromUTF8 (argv[1]))); }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
