#include "MainComponent.h"
#include <JuceHeader.h>

class VoidPlayerApplication : public juce::JUCEApplication
{
public:
    VoidPlayerApplication() {}

    const juce::String getApplicationName() override       { return "Void Software Player v2.∞"; }
    const juce::String getApplicationVersion() override    { return "Phase 1 – Final Polished Build"; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override { mainWindow = nullptr; }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name) : DocumentWindow (name,
                                                        juce::Colours::black,
                                                        DocumentWindow::allButtons)
        {
            setContentOwned (new MainComponent(), true);
            centreWithSize (1000, 700);
            setVisible (true);
            setResizable (true, false);
        }

        void closeButtonPressed() override { JUCEApplication::quit(); }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (VoidPlayerApplication)