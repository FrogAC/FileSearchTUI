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

#include "DirTree.hpp"

namespace fstui {
  using namespace ftxui;

  class DirTreeBase : public ComponentBase {
public:
    DirTreeBase(std::vector<std::wstring> &entries,
                std::vector<short> &depths,
                int &selected,
                Ref<MenuOption> option)
        : entries_(entries), depths_(depths), focused_(selected), option_(std::move(option)) {
      UpdatePrefixsAndDepths();
      state_ = States::FOCUSED;
    }

    Element Render() override {
      Elements elements;
      bool is_menu_focused = Focused();
      boxes_.resize(entries_.size());
      for (int i = 0; i < entries_.size(); i++) {
        bool is_selected = (focused_entry() == int(i)) && is_menu_focused && state_ == States::SELECTED;
        bool is_focused = (focused_ == int(i));

        auto style = is_focused ? (is_selected ? option_->style_selected_focused
                                               : option_->style_selected)
                                 : (is_selected ? option_->style_focused
                                               : option_->style_normal);
        auto focus_management = !is_selected      ? nothing
                                : is_menu_focused ? focus
                                                  : ftxui::select;

        // editing box
        Element elem;
        if (is_focused && state_ == States::EDITING) {
          auto beforePos = inputString.substr(0,inputPosition);
          auto atPos = inputPosition < inputString.size() ? inputString.substr(inputPosition,1) : L" ";
          auto afterPos = inputPosition < (int)inputString.size() - 1 ? inputString.substr(inputPosition+1) : L"";
          elem = hbox(text(prefixs_[i] + beforePos), text(atPos) | underlined, text(afterPos)) | inverted;
        } else {
          elem = text(prefixs_[i] + entries_.at(i));
        }

        elements.emplace_back(elem | style | focus_management | reflect(boxes_[i]));
      }
      return vbox(std::move(elements));
    }

    bool OnEvent(Event event) override {
      // ftxui States
      if (!CaptureMouse(event))
        return false;
//      if (event.is_mouse())
//        return OnMouseEvent(event);
      if (!Focused())
        return false;

      // fstui States
      switch (state_) {
        case States::FOCUSED:
          if (event == Event::ArrowDown) {
            MoveFocus(focused_ + 1);
          } else if (event == Event::ArrowUp) {
            MoveFocus(focused_ - 1);
          } else if (event == Event::Character(' ')) {
            TransitState(States::SELECTED);
          } else if (event == Event::Return) {
            AddEntry(focused_ + 1, depths_.at(focused_)+1);
            MoveFocus(focused_ + 1);
            TransitState(States::EDITING);
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
            MoveDepth(focused_, depths_[focused_]+1);
          } else if (event == Event::ArrowLeft) {
            MoveDepth(focused_, depths_[focused_]-1);
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
            entries_[focused_] = inputString;
            TransitState(States::FOCUSED);
          } else if (event == Event::Escape) {
            TransitState(States::FOCUSED);
          } else if (event.is_character()) {
            inputString.insert(inputPosition, 1, event.character());
            inputPosition++;
            option_->on_change();
          } else if (event == Event::ArrowLeft) {
            if (inputPosition > 0) inputPosition--;
          } else if (event == Event::ArrowRight) {
            if (inputPosition < inputString.size()) inputPosition++;
          } else if (event == Event::Delete) {
            if (inputPosition < inputString.size()) {
              inputString.erase(inputPosition,1);
              option_->on_change();
            }
          } else if (event == Event::Backspace) {
            if (inputPosition > 0) {
              inputPosition--;
              inputString.erase(inputPosition,1);
              option_->on_change();
            }
          } else if (event == Event::Home) {
            inputPosition = 0;
          } else if (event == Event::End) {
            inputPosition = inputString.size();
          } else {
            return false;
          }
          break;
      }
      return true;
    }

    void MoveFocus(int dstId) {
      auto old_selected = focused_;

      focused_ = (dstId + entries_.size()) % entries_.size();

      if (focused_ != old_selected) {
        focused_entry() = focused_;
        option_->on_change();
      }
    }

    void AddEntry(int dstId, short depth = 0, const std::wstring& content = L"") {
      entries_.insert(entries_.begin()+ dstId, content);
      depths_.insert(depths_.begin()+ dstId, depth);
      UpdatePrefixsAndDepths();
    }

    void MoveEntry(int srcId, int dstId) {
      dstId = (dstId + entries_.size()) % entries_.size();
      // swap
      std::iter_swap(entries_.begin()+srcId, entries_.begin()+ dstId);
      std::iter_swap(depths_.begin()+srcId, depths_.begin()+ dstId);
      std::iter_swap(prefixs_.begin()+srcId, prefixs_.begin()+ dstId);
//      // recalc depth
//      depths_.at(dstId) = dstId == 0 ? 0 : std::min(depths_.at(dstId), (short)(1+depths_.at(dstId -1)));
//      depths_.at(srcId) = srcId == 0 ? 0 : std::min(depths_.at(srcId), (short)(1+depths_.at(srcId-1)));
      // reacalc prefix
//      UpdatePrefixsAndDepths();
    }

    void RemoveEntry(int tgtId) {
      if (entries_.size()<=1) return;
      entries_.erase(entries_.begin() + tgtId);
      depths_.erase(depths_.begin() + tgtId);
      UpdatePrefixsAndDepths();

      // selected overflow
      if (focused_ > entries_.size()-1) {
        MoveFocus(focused_ -1);
      }
    }

    void MoveDepth(int entryId, short depth) {
      depths_[entryId] = depth;
      UpdatePrefixsAndDepths(false);
    }

protected:
    std::vector<std::wstring> entries_;
    std::vector<short> depths_;
    int focused_;

    Ref<MenuOption> option_;

    std::vector<Box> boxes_;

private:
    enum States { FOCUSED,
                  SELECTED,
                  EDITING };
    States state_;
    std::vector<std::wstring> prefixs_;

    std::wstring inputString;
    int inputPosition;

    int &focused_entry() { return option_->focused_entry(); }

    // trans states on current high light
    void TransitState(States targetState) {
      if (targetState == state_) return;

      if (targetState == States::EDITING) {
        inputString = entries_[focused_];
        inputPosition = inputString.size();
      }

      UpdatePrefixsAndDepths();
      state_ = targetState;
    }

    // recalc prefixs
    void UpdatePrefixsAndDepths(bool formatDepth = true) {
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
  };

  Component DirTree(std::vector<std::wstring>& entries,
                    std::vector<short>& depths,
                    int& selected,
                    Ref<MenuOption> option) {
    return Make<DirTreeBase>(entries, depths, selected, std::move(option));
  }


}// namespace fstui