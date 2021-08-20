#ifndef FSTUI_DIRTREEBASE_HPP
#define FSTUI_DIRTREEBASE_HPP

#include <filesystem>

#include "ftxui/component/component_base.hpp"   // for component base
#include "ftxui/component/component_options.hpp"// for MenuOption
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/util/ref.hpp"// for Ref

namespace fstui {
  using namespace ftxui;
  namespace fs = std::filesystem;

  class DirTreeBase : public ComponentBase {
public:
    DirTreeBase(std::vector<std::wstring> &entries,
                std::vector<short> &depths,
                int &selected,
                std::vector<ConstStringRef> &labels,
                std::vector<std::vector<bool>> &labelChecks,
                std::string windowName,
                Ref<MenuOption> menuOption = {},
                Ref<CheckboxOption> checkboxOption = {});

    Element Render() override;
    bool OnEvent(Event event) override;
    void Init();

private:
    // STATES
    enum States { FOCUSED,
                  SELECTED,
                  EDITING };
    States state_;
    bool isLabelsFocused_;
    const std::wstring windowName_;

    // DIR TREE
    std::vector<std::wstring> &entries_;
    std::vector<short> &depths_;
    int &focused_;
    std::vector<std::wstring> prefixs_;
    std::vector<Box> treeBoxes_;
    std::wstring inputString_;
    int inputPosition_;

    // LABEL CHECKBOXES
    std::vector<ConstStringRef> &labels_;
    std::vector<std::vector<bool>> &labelChecked_;
    std::vector<Box> labelBoxes_;
    int labelFocused_;
    Ref<CheckboxOption> checkboxOption_;
    Ref<MenuOption> menuOption_;

    int &focused_entry() { return menuOption_->focused_entry(); }
    void TransitState(States targetState);
    bool OnMouseEvent(Event event);

    void ToggleLabel(int dirId, int labelId);
    void MoveFocus(int dstId);
    void MoveLabelFocus(int dstId);
    void AddEntry(int dstId, short depth = 0, const std::wstring &content = L"");
    void MoveEntry(int srcId, int dstId);
    void RemoveEntry(int tgtId);
    void MoveDepth(int entryId, short depth);
    void UpdatePrefixsAndDepths(bool formatDepth = true);
  };
}// namespace fstui

#endif