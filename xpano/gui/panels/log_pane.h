
#include "xpano/log/logger.h"

namespace xpano::gui {

class LogPane {
 public:
  explicit LogPane(logger::Logger* logger);
  void Draw();

 private:
  logger::Logger* logger_;
};

}  // namespace xpano::gui
