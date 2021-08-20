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

  PresetsBase::PresetsBase(std::string presetDir,
                           std::string actionName,
                           const std::string &windowName,
                           const std::function<void(fs::path &)> onSave,
                           const std::function<void(fs::path &)> onLoad,
                           const std::function<void(fs::path &)> onAction)
      : presetDir_(std::move(presetDir)), actionName_(actionName.begin(), actionName.end()), windowName_(windowName.begin(), windowName.end()),
        focused_(0), selected_(0),
        presetPaths_(), presetEntries_(),
        onSave_(onSave), onLoad_(onLoad), onAction_(onAction), menuOption_() {
    // proc preset names
    state_ = States::PRESETS;
    const fs::path p{presetDir_};
    if (!exists(p)) fs::create_directory(p);
    for (auto const &f : fs::directory_iterator{p}) {
      if (f.is_directory() || f.path().extension().string() != presetExt_) continue;
      std::wstring file = f.path().filename().wstring();
      presetEntries_.emplace_back(file);
      presetPaths_.emplace_back(f.path());
    }

    // load first
    if (presetPaths_.size() > 0) {
      onLoad_(presetPaths_[0]);
    }
  }


  Element PresetsBase::Render() {
    // preset list
    Elements elements;
    bool is_menu_focused = PresetsBase::Focused();
    presetBoxes_.resize(presetEntries_.size());
    for (int i = 0; i < presetEntries_.size(); i++) {
      bool is_focused = focused_ == int(i) && state_ == PRESETS;
      bool is_selected = selected_ == int(i) && Focused();

      auto style = is_focused ? (is_selected ? menuOption_.style_selected_focused
                                             : menuOption_.style_selected)
                              : (is_selected ? menuOption_.style_focused
                                             : menuOption_.style_normal);

      auto focus_management = !is_selected      ? nothing
                              : is_menu_focused ? focus
                                                : ftxui::select;

      Element elem;
      elem = text(presetEntries_.at(i));
      elements.emplace_back(elem | style | focus_management | reflect(presetBoxes_[i]));
    }
    // save name
    Element savename;
    if (state_ == States::EDITSAVENAME) {
      auto beforePos = inputString_.substr(0, inputPosition_);
      auto atPos = inputPosition_ < inputString_.size() ? inputString_.substr(inputPosition_, 1) : L" ";
      auto afterPos = inputPosition_ < (int) inputString_.size() - 1 ? inputString_.substr(inputPosition_ + 1) : L"";
      savename = hbox(text(beforePos), text(atPos) | underlined, text(afterPos));
    } else {
      inputString_ = presetEntries_[selected_].substr(0, presetEntries_[selected_].size() - presetExt_.size());
      savename = text(inputString_);
    }
    if (state_ == States::EDITSAVENAME || state_ == States::SAVENAME) savename = savename | inverted;
    savename = hbox(text(L"Name: ") | vcenter, border(savename | reflect(nameBox_)));
    // action btn
    Element actionbtn = border(text(actionName_) | center | (state_ == States::ACTIONBTN ? inverted : nothing)) | hcenter;
    actionbtn = actionbtn | reflect(actionbtnBox_);

    return window(
            text(windowName_),
            vbox(
                    {vbox(std::move(elements)),
                     savename,
                     actionbtn}));
  }

  bool PresetsBase::OnEvent(Event event) {
    if (!CaptureMouse(event))
      return false;
    if (event.is_mouse())
      return OnMouseEvent(event);
    if (!Focused())
      return false;

    switch (state_) {
      case States::PRESETS:
        if (event == Event::ArrowUp) {
          if (focused_ > 0) {
            focused_--;
          }
        } else if (event == Event::ArrowDown) {
          if (focused_ < presetEntries_.size() - 1) {
            focused_++;
          } else {
            state_ = SAVENAME;
          }
        } else if (event == Event::Character(' ') || event == Event::Return) {
          selected_ = focused_;
          onLoad_(presetPaths_[selected_]);
        } else {
          return false;
        }
        break;
      case States::SAVENAME:
        if (event == Event::Return || event == Event::Character(' ')) {
          state_ = States::EDITSAVENAME;
          inputPosition_ = inputString_.size();
        } else if (event == Event::ArrowUp) {
          state_ = States::PRESETS;
        } else if (event == Event::ArrowDown) {
          state_ = States::ACTIONBTN;
        } else {
          return false;
        }
        break;
      case States::EDITSAVENAME:
        if (event == Event::Return) {
          // remote dup
          std::wstring newName = inputString_ + std::wstring(presetExt_.begin(), presetExt_.end());;
          for (auto& e : presetEntries_) {
            if (newName == e) {
              inputString_.append(L"_dup");
              newName = inputString_ + std::wstring(presetExt_.begin(), presetExt_.end());
            }
          }
          // add new entry
          fs::path dir{presetDir_};
          fs::path file{newName};
          fs::path newPath = dir/file;
          presetEntries_.insert(presetEntries_.begin()+selected_+1,newName);
          presetPaths_.insert(presetPaths_.begin()+selected_+1, newPath);
          selected_++;
          focused_++;
          onSave_(presetPaths_[selected_]);
          onLoad_(presetPaths_[selected_]);
          state_ = States::SAVENAME;
        } else if (event == Event::Escape) {
          state_ = States::SAVENAME;
        } else if (event.is_character()) {
          inputString_.insert(inputPosition_, 1, event.character());
          inputPosition_++;
          menuOption_.on_change();
        } else if (event == Event::ArrowLeft) {
          if (inputPosition_ > 0) inputPosition_--;
        } else if (event == Event::ArrowRight) {
          if (inputPosition_ < inputString_.size()) inputPosition_++;
        } else if (event == Event::Delete) {
          if (inputPosition_ < inputString_.size()) {
            inputString_.erase(inputPosition_, 1);
            menuOption_.on_change();
          }
        } else if (event == Event::Backspace) {
          if (inputPosition_ > 0) {
            inputPosition_--;
            inputString_.erase(inputPosition_, 1);
            menuOption_.on_change();
          }
        } else if (event == Event::Home) {
          inputPosition_ = 0;
        } else if (event == Event::End) {
          inputPosition_ = inputString_.size();
        } else {
          return false;
        }
        break;
      case States::ACTIONBTN:
        if (event == Event::ArrowUp) {
          state_ = States::SAVENAME;
        } else if (event == Event::Return || event == Event::Character(' ')) {
          onAction_(presetPaths_[selected_]);
        } else {
          return false;
        }
        break;
    }
    return true;
  }

  bool PresetsBase::OnMouseEvent(Event event) {
    if (!CaptureMouse(event))
      return false;
    if (state_ == States::EDITSAVENAME) return false;
    // presets
    for (int i = 0; i < int(presetBoxes_.size()); ++i) {
      if (!presetBoxes_[i].Contain(event.mouse().x, event.mouse().y))
        continue;

      TakeFocus();
      focused_ = i;
      if (event.mouse().button == Mouse::Left &&
          event.mouse().motion == Mouse::Released) {
        if (selected_ != i) {
          selected_ = i;
          focused_ = i;
          onLoad_(presetPaths_[selected_]);
          return true;
        }
      }
    }
    // save name
    if (nameBox_.Contain(event.mouse().x, event.mouse().y)) {
      TakeFocus();
      if (event.mouse().button == Mouse::Left &&
          event.mouse().motion == Mouse::Released) {
          state_ = States::EDITSAVENAME;
          inputPosition_ = inputString_.size();
          return true;
      }
    }
    // actionbtn
    if (actionbtnBox_.Contain(event.mouse().x, event.mouse().y)) {
      TakeFocus();
      if (event.mouse().button == Mouse::Left &&
          event.mouse().motion == Mouse::Released) {
        state_ = States::ACTIONBTN;
        onAction_(presetPaths_[selected_]);
        return true;
      }
    }
    return false;
  }

}// namespace fstui