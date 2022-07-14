#pragma once

#include <cstdlib>
#include <string>

#include "editor_base.hpp"

namespace clog::editor {

class EnvBasedEditor : public EditorBase {
  public:
    void openEditor(const std::string &path) {
        if (std::getenv("EDITOR")) {
            std::system(("$EDITOR " + path).c_str());
        }
    }
};

}