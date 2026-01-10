#include <JuceHeader.h>
#include "MainComponent.h"

class VoidPlayerApplication : public juce::JUCEApplication
{
public:
    VoidPlayerApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Colours::black,
                              DocumentWindow::allButtons)
        {
            setContentOwned (new MainComponent(), true);
            setUsingNativeTitleBar (true);

            // === WINDOWED + RESPONSIVE ENABLED ===
            setResizable (true, true);                  // Allow resizing with grip
            setResizeLimits (900, 700, 3840, 2160);     // Min size to fit controls, max 4K

            centreWithSize (1280, 900);                 // Start larger & centered

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (VoidPlayerApplication)