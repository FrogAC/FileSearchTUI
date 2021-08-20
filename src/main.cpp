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

  // tree
  std::vector<std::wstring> entries;
  std::vector<short> depths;
  int selected = 0;
  std::vector<ConstStringRef> labels;
  std::vector<std::vector<bool>> labelChecked;
  auto tree = std::make_shared<DirTreeBase>(entries, depths, selected, labels, labelChecked, "Directory Tree");

  auto onSave = [&labels, &entries, &depths, &selected, &labelChecked](fs::path &path) {
    std::wofstream f(path);
    if (labels.size() > 0) {
      f << "|";
      for (auto l : labels) f << *l << "|";
      f << std::endl;
    }
    for (size_t i = 0; i < entries.size(); i++) {
      for (auto j = 0; j < depths[i]; j++) f << '\t';
      f << entries[i];
      if (labels.size() > 0) {
        f << " |";
        for (auto l : labelChecked[i]) f << (l ? '1' : '0');
        f << "|";
      }
      f << std::endl;
    }
    f.close();
  };

  auto onLoad = [&labels, &entries, &depths, &selected, &labelChecked, &tree](fs::path &path) {
    labels = std::vector<ConstStringRef>();
    entries = {};
    depths = {};
    selected = 0;
    labelChecked = {};

    if (!exists(path)) {
      tree->Init();
      return;
    }

    std::ifstream file(path.string());
    std::string line;
    if (file.is_open()) {
      // load labels
      if (getline(file, line)) {
        auto options = str::split(line,'|');
        if (options.size() > 2) {
          for (auto it = options.begin() + 1; it < options.end()-1; it++) labels.push_back({*it});
        }
      } else {
        // no labels used
        file.clear();
        file.seekg(0);
      }
      // load selection
      while(getline(file, line) && line.size() > 0) {
        depths.push_back(std::count(line.begin(), line.end(), '\t'));
        line = str::ltrim(line);
        auto labelPos = line.find(" |");
        auto entry = line.substr(0,labelPos);
        auto label = line.substr(labelPos+2, labels.size());
        std::vector<bool> labelVec;
        for (auto l : label) labelVec.push_back(l == '1');
        entries.push_back(std::wstring(entry.begin(), entry.end()));
        labelChecked.push_back(labelVec);
      }
      file.close();
    }

    tree->Init();
  };

  /*
   * Save to
   */
  auto onAction = [](fs::path &path) {

  };

  auto preset = std::make_shared<PresetsBase>("presets", "---", "Presets", onSave, onLoad, onAction);

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(Container::Horizontal({preset, tree}));

  return 0;
}
