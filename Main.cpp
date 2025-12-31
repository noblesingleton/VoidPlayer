#include <JuceHeader.h>
#include "MainComponent.h"

class VoidPlayerApplication : public juce::JUCEApplication
{
public:
    VoidPlayerApplication() {}

    const juce::String getApplicationName() override { return "VoidPlayer_Clean"; }
    const juce::String getApplicationVersion() override { return "1.0"; }

    void initialise(const juce::String& commandLine) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name) : DocumentWindow(name,
            juce::Colours::black,
            DocumentWindow::allButtons)
        {
            setContentOwned(new MainComponent(), true);
            centreWithSize(600, 400);
            setVisible(true);
            setUsingNativeTitleBar(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(VoidPlayerApplication)