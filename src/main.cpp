#include <memory>  
#include <string>  
#include <vector> 
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "DirTreeBase.hpp"
#include "PresetsBase.hpp"
#include "ftxui/component/component.hpp"// for Make, Menu
#include "stringtoolbox.hpp"

#include "ftxui/component/screen_interactive.hpp"// for Component
#include "ftxui/dom/elements.hpp"                // for operator|, Element, reflect, text, vbox, Elements, focus, nothing, select

int main(int argc, const char* argv[]) {
  using namespace ftxui;
  using namespace fstui;
  namespace fs = std::filesystem;
  namespace str = stringtoolbox;

  // Start App
  auto screen = ScreenInteractive::Fullscreen();

  // tree
  std::vector<std::wstring> entries;
  std::vector<short> depths;
  int selected = 0;
  std::vector<ConstStringRef> labels;
  auto tree = std::make_shared<DirTreeBase>(entries, depths, selected, labels, "Directory Tree");


  // preset
  auto onLoad = [&labels, &entries, &depths, &selected, &tree](fs::path path) {
    if (!exists(path)) return;

    labels = std::vector<ConstStringRef>({"A"});
    entries = {L"A",L"B"};
    depths = {0,1};
    selected = 0;

//    std::ifstream file(path.string());
//    std::string line;
//    if (file.is_open()) {
//      if (getline(file, line)) {
//        auto options = str::split(line,'|');
//        options[0] = options[0].erase(0,1);
//        options[options.size()-1] = options[options.size()-1].erase(options.size()-1);
//        for (const auto& o : options) labels.push_back({o});
//      }
//      while(getline(file, line) && line.size() > 0) {
//        depths.push_back(std::count(line.begin(), line.end(), '\t'));
//        line = str::ltrim(line);
//        entries.push_back(std::wstring(line.begin(), line.end()));
//      }
//      file.close();
//    }

    tree->Init();
  };

  auto preset = std::make_shared<PresetsBase>("presets", "Search");

  screen.Loop(Container::Horizontal({preset, tree}));

  return 0;
}
