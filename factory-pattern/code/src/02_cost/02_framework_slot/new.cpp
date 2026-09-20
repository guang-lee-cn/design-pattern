// src/02_cost/02_framework_slot/new.cpp
//
// Same scenario with the Factory Method pattern -- its original GoF context.
// The framework owns the flow (Dialog::Render) and defines an abstract
// product (Button) plus a factory slot (CreateButton). Which concrete
// product is instantiated is decided by subclasses on the application side.
// The framework never names a concrete button.
#include <cstdio>
#include <memory>

namespace {

// -- Framework side: abstractions only --
class Button {
public:
    virtual ~Button()           = default;
    virtual void Render() const = 0;
};

class Dialog {
public:
    virtual ~Dialog() = default;

    // Factory method: the slot subclasses fill in.
    virtual std::unique_ptr<Button> CreateButton() const = 0;

    // Framework flow, fixed once: a template method built on the factory.
    void Render() const {
        std::unique_ptr<Button> button = CreateButton();
        button->Render();
    }
};

// -- Application side: concrete products paired with concrete creators --
class WindowsButton : public Button {
public:
    void Render() const override {
        std::printf("[Windows] native button rendered\n");
    }
};

class WindowsDialog : public Dialog {
public:
    std::unique_ptr<Button> CreateButton() const override {
        return std::make_unique<WindowsButton>();
    }
};

class HtmlButton : public Button {
public:
    void Render() const override {
        std::printf("[Web] HTML <button> rendered\n");
    }
};

class WebDialog : public Dialog {
public:
    std::unique_ptr<Button> CreateButton() const override {
        return std::make_unique<HtmlButton>();
    }
};

} // namespace

int main() {
    WindowsDialog windows;
    windows.Render(); // framework flow + application-supplied product
    WebDialog web;
    web.Render();
}
