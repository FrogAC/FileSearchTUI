#ifndef FSTUI_PRESETSBASE_HPP
#define FSTUI_PRESETSBASE_HPP

#include <filesystem>

#include "ftxui/component/component_base.hpp"// for component base
#include "ftxui/component/component_options.hpp"// for MenuOption
#include "ftxui/component/screen_interactive.hpp"

namespace fstui {
  using namespace ftxui;
  namespace fs = std::filesystem;

  class PresetsBase : public ComponentBase {
public:
    PresetsBase(const std::string presetDir,
                  const std::string action,
                  std::function<void(fs::path)> onSave = {},
                  std::function<void(fs::path)> onLoad = {},
                  std::function<void(fs::path)> onAction = {});

    Element Render() override;
    bool OnEvent(Event event) override;

private:
    const std::string presetDir_;
    std::vector<std::wstring> presetEntries_;
    std::vector<fs::path> presetPaths_;
    int focused_;
    int selected_;
    std::vector<Box> presetBoxes_;
    MenuOption menuOption_;
    const std::string action_;
    std::function<void(fs::path)> onSave_;
    std::function<void(fs::path)> onLoad_;
    std::function<void(fs::path)> onAction_;

    bool OnMouseEvent(Event event);
  };
}// namespace fstui

#endif