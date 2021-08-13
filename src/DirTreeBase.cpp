#include <algorithm> // for max, min
#include <functional>// for function
#include <memory>    // for shared_ptr, allocator_traits<>::value_type
#include <stddef.h>  // for size_t
#include <string>    // for operator+, wstring
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

#include "DirTreeBase.hpp"

namespace fstui {
  using namespace ftxui;


  DirTreeBase::DirTreeBase(std::vector<std::wstring> &entries,
              std::vector<short> &depths,
              int &selected,
              std::vector<ConstStringRef> &itemLabels,
              std::string windowName,
              Ref<MenuOption> menuOption,
              Ref<CheckboxOption> checkboxOption)
      : entries_(entries), depths_(depths), focused_(selected),
        labels_(itemLabels), windowName_(windowName),
        menuOption_(std::move(menuOption)), checkboxOption_(std::move(checkboxOption)) {
    UpdatePrefixsAndDepths();
    state_ = States::FOCUSED;
    isLabelsFocused_ = false;
    labelBoxes_.resize(labels_.size());
    labelFocused_ = 0;
    labelChecked_ = std::vector<std::vector<bool>>(entries_.size(), std::vector<bool>(labels_.size(), false));
    // force checkbox style
    checkboxOption_->style_checked = L"[X]";
    checkboxOption_->style_unchecked = L"[ ]";
  }

  Element DirTreeBase::Render() {
    UpdatePrefixsAndDepths();// TODO better performance

    Elements elements;
    bool is_menu_focused = Focused();
    treeBoxes_.resize(entries_.size());
    for (int i = 0; i < entries_.size(); i++) {
      bool is_selected = (focused_entry() == int(i)) && is_menu_focused && state_ == States::SELECTED;
      bool is_focused = (focused_ == int(i));

      auto style = is_focused ? (is_selected ? menuOption_->style_selected_focused
                                             : menuOption_->style_selected)
                              : (is_selected ? menuOption_->style_focused
                                             : menuOption_->style_normal);
      auto focus_management = !is_selected      ? nothing
                              : is_menu_focused ? focus
                                                : ftxui::select;

      // EDITION MODE
      Element elem;
      if (is_focused && state_ == States::EDITING) {
        auto beforePos = inputString_.substr(0, inputPosition_);
        auto atPos = inputPosition_ < inputString_.size() ? inputString_.substr(inputPosition_, 1) : L" ";
        auto afterPos = inputPosition_ < (int) inputString_.size() - 1 ? inputString_.substr(inputPosition_ + 1) : L"";
        elem = hbox(text(prefixs_[i] + beforePos), text(atPos) | underlined, text(afterPos)) | inverted;
      } else {
        elem = text(prefixs_[i] + entries_.at(i));
      }

      elements.emplace_back(elem | style | focus_management | reflect(treeBoxes_[i]));
    }

    // FOCUSED -> RIGHT PANEL
    Elements labels;
    Elements arrows;
    //      if (state_ == States::FOCUSED) {
    int padding = std::min(focused_, (int) (entries_.size() - labels_.size()));
    // space before
    for (int i = 0; i < padding; i++) {
      labels.emplace_back(text(L""));
      arrows.emplace_back(text(L""));
    }
    for (int i = 0; i <= focused_ - padding; i++) {
      arrows.emplace_back(text(L""));
    }
    arrows.emplace_back(text(L" > "));
    // labels
    for (int i = 0; i < labels_.size(); i++) {
      bool is_focused = isLabelsFocused_ && labelFocused_ == i;
      auto style = is_focused ? checkboxOption_->style_focused : checkboxOption_->style_unfocused;
      auto focus_management = is_focused ? focus : labelChecked_[focused_][i] ? ftxui::select
                                                                              : ftxui::nothing;
      labels.emplace_back(hbox(text(labelChecked_[focused_][i] ? checkboxOption_->style_checked
                                                               : checkboxOption_->style_unchecked),
                               text(*labels_[i]) | style | focus_management) |
                          reflect(labelBoxes_[i]));
    }
    //      }

    return window(
            text(L"Directory"),
            hbox({border(vbox(std::move(elements))),
                  vbox(std::move(arrows)),
                  border(vbox(std::move(labels)))}));
  }

  bool DirTreeBase::OnEvent(Event event) {
    // ftxui States
    if (!CaptureMouse(event))
      return false;
    if (event.is_mouse())
      return OnMouseEvent(event);
    if (!Focused())
      return false;

    // fstui States
    switch (state_) {
      case States::FOCUSED:
        if (event == Event::ArrowDown) {
          if (isLabelsFocused_)
            MoveLabelFocus(labelFocused_ + 1);
          else
            MoveFocus(focused_ + 1);
        } else if (event == Event::ArrowUp) {
          if (isLabelsFocused_)
            MoveLabelFocus(labelFocused_ - 1);
          else
            MoveFocus(focused_ - 1);
        } else if (event == Event::ArrowRight) {
          if (!isLabelsFocused_)
            isLabelsFocused_ = true;
          else
            return false;// fallback to container
        } else if (event == Event::ArrowLeft) {
          if (isLabelsFocused_)
            isLabelsFocused_ = false;
          else
            return false;// fallback to container
        } else if (event == Event::Character(' ') || event == Event::Return) {
          if (isLabelsFocused_) {// right panel
            ToggleLabel(focused_, labelFocused_);
          } else {// left panel
            if (event == Event::Character(' ')) {
              TransitState(States::SELECTED);
            } else {
              AddEntry(focused_ + 1, depths_.size() > 0 ? depths_.at(focused_) + 1 : 0);
              MoveFocus(focused_ + 1);
              TransitState(States::EDITING);
            }
          }
        } else if (event == Event::Backspace || event == Event::Delete) {
          RemoveEntry(focused_);
        } else {
          return false;
        }
        break;
      case States::SELECTED:
        if (event == Event::ArrowDown) {
          MoveEntry(focused_, focused_ + 1);
          MoveFocus(focused_ + 1);
        } else if (event == Event::ArrowUp) {
          MoveEntry(focused_, focused_ - 1);
          MoveFocus(focused_ - 1);
        } else if (event == Event::ArrowRight) {
          MoveDepth(focused_, depths_[focused_] + 1);
        } else if (event == Event::ArrowLeft) {
          MoveDepth(focused_, depths_[focused_] - 1);
        } else if (event == Event::Character(' ')) {
          TransitState(States::FOCUSED);
        } else if (event == Event::Return) {
          TransitState(States::EDITING);
        } else if (event == Event::Backspace || event == Event::Delete) {
          RemoveEntry(focused_);
          TransitState(States::FOCUSED);
        } else {
          return false;
        }
        break;
      case States::EDITING:
        if (event == Event::Return) {
          entries_[focused_] = inputString_;
          TransitState(States::FOCUSED);
        } else if (event == Event::Escape) {
          TransitState(States::FOCUSED);
        } else if (event.is_character()) {
          inputString_.insert(inputPosition_, 1, event.character());
          inputPosition_++;
          menuOption_->on_change();
        } else if (event == Event::ArrowLeft) {
          if (inputPosition_ > 0) inputPosition_--;
        } else if (event == Event::ArrowRight) {
          if (inputPosition_ < inputString_.size()) inputPosition_++;
        } else if (event == Event::Delete) {
          if (inputPosition_ < inputString_.size()) {
            inputString_.erase(inputPosition_, 1);
            menuOption_->on_change();
          }
        } else if (event == Event::Backspace) {
          if (inputPosition_ > 0) {
            inputPosition_--;
            inputString_.erase(inputPosition_, 1);
            menuOption_->on_change();
          }
        } else if (event == Event::Home) {
          inputPosition_ = 0;
        } else if (event == Event::End) {
          inputPosition_ = inputString_.size();
        } else {
          return false;
        }
        break;
    }
    return true;
  }

  bool DirTreeBase::OnMouseEvent(Event event) {
    if (!CaptureMouse(event))
      return false;
    for (int i = 0; i < int(treeBoxes_.size()); ++i) {
      if (!treeBoxes_[i].Contain(event.mouse().x, event.mouse().y))
        continue;

      TakeFocus();
      isLabelsFocused_ = false;
      if (event.mouse().button == Mouse::Left &&
          event.mouse().motion == Mouse::Released) {
        if (focused_ != i) {
          if (state_ == States::SELECTED || state_ == States::EDITING) MoveEntry(focused_, i);
          MoveFocus(i);
          return true;
        }
      }
    }

    // CHECKBOX FOCUS
    if (state_ == States::FOCUSED) {
      for (int i = 0; i < int(labelBoxes_.size()); i++) {
        if (!labelBoxes_[i].Contain(event.mouse().x, event.mouse().y)) continue;

        TakeFocus();
        isLabelsFocused_ = true;
        MoveLabelFocus(i);
        if (event.mouse().button == Mouse::Left &&
            event.mouse().motion == Mouse::Pressed) {
          labelChecked_[focused_][i] = !labelChecked_[focused_][i];
          checkboxOption_->on_change();
          return true;
        }
      }
    }
    return false;
  }

  void DirTreeBase::ToggleLabel(int dirId, int labelId) {
    labelChecked_[dirId][labelId] = !labelChecked_[dirId][labelId];
    checkboxOption_->on_change();
  }

  void DirTreeBase::MoveFocus(int dstId) {
    if (entries_.size() == 0) return;

    auto old_selected = focused_;

    focused_ = (dstId + entries_.size()) % entries_.size();

    if (focused_ != old_selected) {
      focused_entry() = focused_;
      menuOption_->on_change();
    }
  }

  void DirTreeBase::MoveLabelFocus(int dstId) {
    auto old_selected = labelFocused_;

    labelFocused_ = (dstId + labels_.size()) % labels_.size();

    if (labelFocused_ != old_selected) {
      menuOption_->on_change();
    }
  }

  void DirTreeBase::AddEntry(int dstId, short depth, const std::wstring &content) {
    entries_.insert(entries_.begin() + dstId, content);
    depths_.insert(depths_.begin() + dstId, depth);
    labelChecked_.insert(labelChecked_.begin() + dstId, std::vector<bool>(labels_.size(), false));
    UpdatePrefixsAndDepths();
  }

  void DirTreeBase::MoveEntry(int srcId, int dstId) {
    if (entries_.size() == 0) return;

    dstId = (dstId + entries_.size()) % entries_.size();
    // swap
    std::iter_swap(entries_.begin() + srcId, entries_.begin() + dstId);
    std::iter_swap(depths_.begin() + srcId, depths_.begin() + dstId);
    std::iter_swap(prefixs_.begin() + srcId, prefixs_.begin() + dstId);
    std::iter_swap(labelChecked_.begin() + srcId, labelChecked_.begin() + dstId);
  }

  void DirTreeBase::RemoveEntry(int tgtId) {
    if (entries_.size() <= 1) return;
    entries_.erase(entries_.begin() + tgtId);
    depths_.erase(depths_.begin() + tgtId);
    labelChecked_.erase(labelChecked_.begin() + tgtId);
    UpdatePrefixsAndDepths();

    // selected overflow
    if (focused_ > entries_.size() - 1) {
      MoveFocus(focused_ - 1);
    }
  }

  void DirTreeBase::MoveDepth(int entryId, short depth) {
    if (entries_.size() == 0) return;

    depths_[entryId] = depth;
    UpdatePrefixsAndDepths(false);
  }

  void DirTreeBase::TransitState(States targetState) {
    if (targetState == state_) return;

    if (targetState == States::EDITING) {
      inputString_ = entries_[focused_];
      inputPosition_ = inputString_.size();
    }

    UpdatePrefixsAndDepths();
    state_ = targetState;
  }

  // recalc prefixs
  void DirTreeBase::UpdatePrefixsAndDepths(bool formatDepth) {
    // depths
    if (formatDepth) {
      for (auto it = depths_.begin() + 1; it < depths_.end(); it++) {
        if (*it > 1 + *(it - 1)) *it = 1 + *(it - 1);
      }
    }

    // prefixs
    prefixs_ = std::vector<std::wstring>(entries_.size());

    std::vector<bool> depthVisible(*std::max_element(depths_.begin(), depths_.end()),
                                   false);

    auto nextDepth = depths_.back();
    for (int i = entries_.size() - 1; i >= 0; i--) {
      auto curDepth = depths_.at(i);
      // jump branch
      if (nextDepth < curDepth) {
        for (auto it = depthVisible.begin() + nextDepth + 1; it <= depthVisible.begin() + curDepth; it++) {
          *it = false;
        }
      }
      nextDepth = curDepth;

      prefixs_[i] = L"";
      if (curDepth > 0) {
        for (int j = 0; j < curDepth - 1; j++) {
          prefixs_[i].append(depthVisible[j + 1] ? L"│   " : L"    ");
        }
        prefixs_[i].append(depthVisible[curDepth] ? L"├───" : L"└───");
        depthVisible[curDepth] = true;
      }
    }
  }


}// namespace fstui