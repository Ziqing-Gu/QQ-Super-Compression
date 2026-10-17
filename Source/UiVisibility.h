#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace qqsc::ui
{
// Message thread only. Component::isShowing() walks JUCE parents, but an
// embedded plugin also has native host ancestors. In particular, hiding a
// Windows host window need not clear any of the retained JUCE visible flags.
// The peer's native visibility query covers that extra boundary.
inline bool isActuallyShowing(const juce::Component& component)
{
    if (!component.isShowing())
        return false;
    if (const auto* peer = component.getPeer())
        return peer->isShowing();
    return false;
}
}
