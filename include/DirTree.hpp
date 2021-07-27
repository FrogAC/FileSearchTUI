#ifndef DIR_TREE_HPP
#define DIR_TREE_HPP

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component_options.hpp"  // for MenuOption
#include "ftxui/util/ref.hpp"    // for Ref

namespace fstui{
using namespace ftxui;

Component DirTree(std::vector<std::wstring>& entries,
                  std::vector<short>& depths,
                  int& selected,
                  Ref<MenuOption> option = {});

} // fstui

#endif