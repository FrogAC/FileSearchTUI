# --- FTXUI --------------------------------------------------------------
CPMAddPackage(
    NAME FTXUI
    GITHUB_REPOSITORY ArthurSonzogni/FTXUI
    GIT_TAG 2f9563a91b7e5d35b4b595559eb2a60a4e691d71   # master
    OPTIONS
      "FTXUI_BUILD_EXAMPLES off"
      "FTXUI_ENABLE_INSTALL off"
      "FTXUI_BUILD_TESTS off"
)
# ------------------------------------------------------------------------------

add_executable(fstui main.cpp DirTree.cpp)

target_link_libraries(fstui
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

# install(TARGETS fstui RUNTIME DESTINATION "bin")
