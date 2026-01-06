#include <JuceHeader.h>
#include "MainComponent.h"

class VoidPlayerApplication : public juce::JUCEApplication
{
public:
    VoidPlayerApplication() {}

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }

    void initialise (const juce::String& commandLine) override
    {
        // Create the main window
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;  // This is automatically deleted when the window is closed
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name) : DocumentWindow (name,
                                                         juce::Colours::black,
                                                         DocumentWindow::allButtons)
        {
            setContentOwned (new MainComponent(), true);
            setUsingNativeTitleBar (true);
            centreWithSize (800, 600);
            setVisible (true);
            setResizable (true, true);
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