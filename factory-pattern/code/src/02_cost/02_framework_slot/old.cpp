// src/02_cost/02_framework_slot/old.cpp
//
// Scenario: a UI framework renders dialogs. Two skins exist:
// a native Windows look and an HTML/web look.
//
// Without the Factory Method pattern, the framework base class must know
// every concrete product. Adding a third skin means editing the framework
// flow itself -- the framework and the application's products are coupled.
#include <cstdio>

namespace {

// -- Concrete products, conceptually owned by the application side --
class WindowsButton {
public:
    void Render() const {
        std::printf("[Windows] native button rendered\n");
    }
};

class HtmlButton {
public:
    void Render() const {
        std::printf("[Web] HTML <button> rendered\n");
    }
};

// -- "Framework": the base class hard-codes both products --
enum class DialogKind { kWindows, kWeb };

class Dialog {
public:
    explicit Dialog(DialogKind kind) : kind_(kind) {
    }

    // The framework flow carries a branch per concrete product and must
    // compile against (include) every product class.
    void Render() const {
        switch (kind_) {
            case DialogKind::kWindows: {
                WindowsButton button;
                button.Render();
                break;
            }
            case DialogKind::kWeb: {
                HtmlButton button;
                button.Render();
                break;
            }
        }
    }

private:
    DialogKind kind_;
};

} // namespace

int main() {
    Dialog windows(DialogKind::kWindows);
    windows.Render();
    Dialog web(DialogKind::kWeb);
    web.Render();
}
