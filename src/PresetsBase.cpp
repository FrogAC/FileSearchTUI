#include <algorithm>// for max, min
#include <filesystem>
#include <functional>// for function
#include <memory>    // for shared_ptr, allocator_traits<>::value_type
#include <stddef.h>  // for size_t
#include <string>    // for operator+, string
#include <utility>   // for move
#include <vector>    // for vector, __alloc_traits<>::value_type

#include "ftxui/component/captured_mouse.hpp"    // for CapturedMouse
#include "ftxui/component/component.hpp"         // for Make, Menu
#include "ftxui/component/component_base.hpp"    // for ComponentBase
#include "ftxui/component/component_options.hpp" // for MenuOption
#include "ftxui/component/event.hpp"             // for Event, Event::ArrowDown, Event::ArrowUp, Event::Return, Event::Tab, Event::TabReverse
#include "ftxui/component/mouse.hpp"             // for Mouse, Mouse::Left, Mouse::Released
#include "ftxui/component/screen_interactive.hpp"// for Component
#include "ftxui/dom/elements.hpp"                // for operator|, Element, reflect, text, vbox, Elements, focus, nothing, select
#include "ftxui/screen/box.hpp"                  // for Box
#include "ftxui/util/ref.hpp"                    // for Ref

#include "PresetsBase.hpp"

namespace fstui {
  using namespace ftxui;
  namespace fs = std::filesystem;

  PresetsBase::PresetsBase(const std::string presetDir,
                           const std::string action,
                           std::function<void(fs::path)> onSave,
                           std::function<void(fs::path)> onLoad,
                           std::function<void(fs::path)> onAction)
      : presetDir_(presetDir), action_(action), focused_(0), selected_(0),
        onSave_(onSave), onLoad_(onLoad), onAction_(onAction_) {
    // proc preset names
    presetEntries_ = std::vector<std::wstring>();
    presetPaths_ = std::vector<fs::path>();
    const fs::path p{presetDir_};
    if (!exists(p)) fs::create_directory(p);
    for (auto const &f : fs::directory_iterator{p}) {
      if (f.is_directory() || f.path().extension().string() != ".df") continue;
      std::wstring file = f.path().filename().wstring();
      presetEntries_.push_back(file);
      presetPaths_.push_back(f.path());
    }
    menuOption_ = {};

    // load first
    if (presetPaths_.size() > 0) {
      onLoad_(presetPaths_.at(focused_));
    }
  }


  Element PresetsBase::Render() {
    // preset list
    Elements elements;
    bool is_menu_focused = PresetsBase::Focused();
    presetBoxes_.resize(presetEntries_.size());
    for (int i = 0; i < presetEntries_.size(); i++) {
      bool is_focused = focused_ == int(i);
      bool is_selected = selected_ == int(i);

      auto style = is_focused ? (is_selected ? menuOption_.style_selected_focused
                                             : menuOption_.style_selected)
                              : (is_selected ? menuOption_.style_focused
                                             : menuOption_.style_normal);

      auto focus_management = !is_selected      ? nothing
                              : is_menu_focused ? focus
                                                : ftxui::select;

      // EDITION MODE
      Element elem;
      elem = text(presetEntries_.at(i));

      elements.emplace_back(elem | style | focus_management | reflect(presetBoxes_[i]));
    }
    return window(
            text(L"Presets"),
            vbox(std::move(elements)));
  }

  bool PresetsBase::OnEvent(Event event) {
    return false;
  }

  bool PresetsBase::OnMouseEvent(Event event) {
    if (!PresetsBase::CaptureMouse(event))
      return false;

    return false;
  }

}// namespace fstui