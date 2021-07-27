#include <memory>  
#include <string>  
#include <vector> 
 
#include "DirTree.hpp"


int main(int argc, const char* argv[]) {
  using namespace ftxui;
  using namespace fstui;

  // Start App
  auto screen = ScreenInteractive::Fullscreen();

  std::vector<std::wstring> entries = {
      L"src",
        L"a",
        L"b",
          L"bb",
            L"bbb",
            L"bbb",
          L"bb",
            L"bbb",
        L"c",
          L"cc"        
  };
  std::vector<short> depths = {
    0,1,1,2,3,3,2,3,1,2
  };
  int selected = 0;
  auto tree = DirTree(entries, depths, selected);
  screen.Loop(tree);

  return 0;
}
