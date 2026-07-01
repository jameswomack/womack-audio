#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace WomackSkin
{
inline juce::Colour editorBackground()    { return juce::Colour (0xff05070d); }
inline juce::Colour editorBackgroundTop() { return juce::Colour (0xff111a28); }
inline juce::Colour panelBackground()     { return juce::Colour (0xff121926); }
inline juce::Colour panelInnerShadow()    { return juce::Colour (0xff080d15); }
inline juce::Colour displayBackground()   { return juce::Colour (0xff09111b); }
inline juce::Colour displayGlass()        { return juce::Colour (0x221b314f); }
inline juce::Colour textStrong()          { return juce::Colour (0xffedf5ff); }
inline juce::Colour textMuted()           { return juce::Colour (0xff8da0bc); }
inline juce::Colour accentCopper()        { return juce::Colour (0xffd28c42); }
inline juce::Colour accentTeal()          { return juce::Colour (0xff41b89d); }
inline juce::Colour accentBlue()          { return juce::Colour (0xff7b9bff); }
inline juce::Colour accentIce()           { return juce::Colour (0xff6fe0ff); }
inline juce::Colour accentAmber()         { return juce::Colour (0xffffb454); }

inline void configureKnob (juce::Slider& slider, juce::Component* popupParent, const juce::String& suffix = {})
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, popupParent);

    if (suffix.isNotEmpty())
        slider.setTextValueSuffix (suffix);
}

inline void configureLabel (juce::Label& label)
{
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, textMuted());
    label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    label.setInterceptsMouseClicks (false, false);
}

inline void paintEditorBackground (juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour accent)
{
    auto area = bounds.toFloat();

    juce::ColourGradient background (editorBackgroundTop(), area.getCentreX(), area.getY(),
                                     editorBackground(), area.getCentreX(), area.getBottom(), false);
    g.setGradientFill (background);
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.035f));
    for (int y = bounds.getY() + 20; y < bounds.getBottom(); y += 28)
        g.drawHorizontalLine (y, area.getX() + 16.0f, area.getRight() - 16.0f);

    g.setColour (accent.withAlpha (0.10f));
    g.drawRoundedRectangle (area.reduced (2.0f), 18.0f, 1.0f);
}

inline void paintPanelShell (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent)
{
    juce::ColourGradient fill (panelBackground().brighter (0.08f), bounds.getCentreX(), bounds.getY(),
                               panelInnerShadow(), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 18.0f);

    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 18.0f, 2.0f);

    g.setColour (accent.withAlpha (0.75f));
    g.drawRoundedRectangle (bounds.reduced (1.5f), 18.0f, 1.2f);

    auto sheen = bounds.removeFromTop (bounds.getHeight() * 0.24f);
    juce::ColourGradient glass (juce::Colours::white.withAlpha (0.08f), sheen.getCentreX(), sheen.getY(),
                                juce::Colours::transparentBlack, sheen.getCentreX(), sheen.getBottom(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (sheen, 16.0f);
}

inline void paintDisplayWell (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent)
{
    juce::ColourGradient fill (displayGlass(), bounds.getCentreX(), bounds.getY(),
                               displayBackground(), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 14.0f);

    g.setColour (accent.withAlpha (0.18f));
    g.fillRoundedRectangle (bounds.reduced (1.0f), 14.0f);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (bounds, 14.0f, 1.2f);

    g.setColour (accent.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (1.0f), 14.0f, 1.0f);
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit LookAndFeel (juce::Colour accentToUse) : accent (accentToUse)
    {
        setColour (juce::PopupMenu::backgroundColourId, displayBackground());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.18f));
        setColour (juce::PopupMenu::highlightedTextColourId, textStrong());
        setColour (juce::PopupMenu::textColourId, textStrong());
        setColour (juce::ComboBox::backgroundColourId, displayBackground());
        setColour (juce::ComboBox::outlineColourId, accent.withAlpha (0.55f));
        setColour (juce::ComboBox::textColourId, textStrong());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (16.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto angle = juce::jmap (sliderPos, 0.0f, 1.0f, rotaryStartAngle, rotaryEndAngle);

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (bounds.translated (0.0f, 4.0f));

        juce::ColourGradient housing (juce::Colour (0xff6c7686), centre.x, bounds.getY(),
                                      juce::Colour (0xff1c2532), centre.x, bounds.getBottom(), false);
        g.setGradientFill (housing);
        g.fillEllipse (bounds);

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (bounds.reduced (1.5f), 1.2f);

        auto ringBounds = bounds.reduced (radius * 0.18f);
        auto start = rotaryStartAngle + 0.12f;
        auto end = rotaryEndAngle - 0.12f;
        auto valueAngle = juce::jlimit (start, end, juce::jmap (sliderPos, 0.0f, 1.0f, start, end));

        juce::Path backgroundArc;
        backgroundArc.addCentredArc (centre.x, centre.y, ringBounds.getWidth() * 0.5f, ringBounds.getHeight() * 0.5f,
                                     0.0f, start, end, true);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.strokePath (backgroundArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, ringBounds.getWidth() * 0.5f, ringBounds.getHeight() * 0.5f,
                                0.0f, start, valueAngle, true);
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto pointerLength = radius * 0.62f;
        auto pointerThickness = juce::jmax (2.0f, radius * 0.10f);
        juce::Path pointer;
        pointer.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (textStrong());
        g.fillPath (pointer);

        g.setColour (accent.withAlpha (0.55f));
        g.fillEllipse (centre.x - radius * 0.12f, centre.y - radius * 0.12f, radius * 0.24f, radius * 0.24f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.5f, 6.0f);
        auto lamp = bounds.removeFromLeft (bounds.getHeight()).reduced (2.0f);

        g.setColour (displayBackground().brighter (0.06f));
        g.fillRoundedRectangle (bounds, bounds.getHeight() * 0.5f);
        g.setColour (accent.withAlpha (0.45f));
        g.drawRoundedRectangle (bounds, bounds.getHeight() * 0.5f, 1.0f);

        g.setColour (button.getToggleState() ? accent : juce::Colour (0xff2a3444));
        g.fillEllipse (lamp);
        g.setColour (juce::Colours::white.withAlpha (button.getToggleState() ? 0.28f : 0.08f));
        g.drawEllipse (lamp, 1.0f);

        g.setColour (button.getToggleState() ? textStrong() : textMuted());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft ((int) std::round (lamp.getRight()) + 8),
                          juce::Justification::centredLeft, 1);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
        paintDisplayWell (g, bounds, accent);

        juce::Path arrow;
        auto arrowBounds = bounds.removeFromRight (26.0f).reduced (8.0f, 10.0f);
        arrow.startNewSubPath (arrowBounds.getX(), arrowBounds.getY());
        arrow.lineTo (arrowBounds.getCentreX(), arrowBounds.getBottom());
        arrow.lineTo (arrowBounds.getRight(), arrowBounds.getY());
        g.setColour (accent);
        g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (13.0f, juce::Font::bold));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (1, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }

private:
    juce::Colour accent;
};
}
