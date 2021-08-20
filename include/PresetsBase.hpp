#ifndef FSTUI_PRESETSBASE_HPP
#define FSTUI_PRESETSBASE_HPP

#include <filesystem>

#include "ftxui/component/component_base.hpp"   // for component base
#include "ftxui/component/component_options.hpp"// for MenuOption
#include "ftxui/component/screen_interactive.hpp"

namespace fstui {
  using namespace ftxui;
  namespace fs = std::filesystem;

  class PresetsBase : public ComponentBase {
public:
    PresetsBase(std::string presetDir,
                std::string actionName,
                const std::string &windowName,
                std::function<void(fs::path&)> onSave,
                std::function<void(fs::path&)> onLoad,
                std::function<void(fs::path&)> onAction);

    Element Render() override;
    bool OnEvent(Event event) override;

private:
    enum States { PRESETS,
                  SAVENAME,
                  EDITSAVENAME,
                  ACTIONBTN};
    States state_;

    // presets menu
    const std::string presetExt_ = ".df";
    const std::string presetDir_;
    std::vector<std::wstring> presetEntries_;
    std::vector<fs::path> presetPaths_;
    int focused_;
    int selected_;
    std::vector<Box>  presetBoxes_;
    MenuOption menuOption_;

    // save name
    std::wstring inputString_;
    int inputPosition_;
    Box nameBox_;

    // action btn
    Box actionbtnBox_;
    const std::wstring actionName_;

    const std::function<void(fs::path&)> onSave_;
    const std::function<void(fs::path&)> onLoad_;
    const std::function<void(fs::path&)> onAction_;

    const std::wstring windowName_;

    bool OnMouseEvent(Event event);
  };
}// namespace fstui

#endif