
#include <imgui.h>

#include "gui/action.h"

namespace xpano::gui {

enum class ShortcutType { kOpen, kExport, kDebug };

const char* Label(ShortcutType type);

Action CheckKeybindings();

}  // namespace xpano::gui