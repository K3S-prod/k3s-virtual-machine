#include "testgen/runtime_image.h"

namespace k3s {

void Interpreter::FetchHook(void *rt_image) const {
    auto this_ = reinterpret_cast<testgen::RuntimeImage *>(rt_image);
    this_->Hook();
}

}
