#include "xpano/pipeline/options.h"

namespace xpano::pipeline {

const char *Label(ChromaSubsampling subsampling) {
  switch (subsampling) {
    case ChromaSubsampling::k444:
      return "Off";
    case ChromaSubsampling::k422:
      return "Half";
    case ChromaSubsampling::k420:
      return "Quarter";
    default:
      return "Unknown";
  }
}

}  // namespace xpano::pipeline
