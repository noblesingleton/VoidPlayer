#include <JuceHeader.h>

class VoidPlayerApplication : public juce::JUCEApplication
{
public:
    VoidPlayerApplication() {}

    const juce::String getApplicationName() override       { return "Void Software Player v2.∞"; }
    const juce::String getApplicationVersion() override    { return "Eternal Build"; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override { mainWindow = nullptr; }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name) : DocumentWindow (name, juce::Colours::black, DocumentWindow::allButtons)
        {
            setContentOwned (new juce::AudioAppComponent(), true);
            centreWithSize (800, 600);
            setVisible (true);
        }

        void closeButtonPressed() override { JUCEApplication::quit(); }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (VoidPlayerApplication)
