# Factory Presets

Four factory presets for Womack FX in JUCE APVTS XML format.

## Installing in Logic Pro

1. Open Logic Pro with Womack FX inserted on a track.
2. In the plugin window, click the preset menu (top center of the AU window).
3. Choose **"Import Settings…"** and navigate to a `.xml` file from this folder.

## Manual parameter import

If Logic Pro does not show an import option, use the Standalone app:
1. Open `Womack FX.app` (in `/Applications` or the build folder).
2. Right-click the plugin area → **Load state from XML file** (future: add this button to the UI).

## Format

Each file is the raw APVTS state XML. The root element is `<Parameters>` with one attribute per parameter ID matching `Source/Parameters.h`.
