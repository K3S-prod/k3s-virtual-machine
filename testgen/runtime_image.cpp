#include "testgen/runtime_image.h"
#include <random>

namespace k3s {

void Interpreter::FetchHook(void *rt_image) const {
    auto this_ = reinterpret_cast<testgen::RuntimeImage *>(rt_image);
    this_->Hook();
}

namespace testgen {

static double fRand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

void RuntimeImage::CreateNumInConstantPool(uint8_t constant_pool_id, double value) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    Runtime::GetConstantPool()->SetNum(constant_pool_id, value);
}

void RuntimeImage::CreateRandNumInConstantPool(uint8_t constant_pool_id) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    auto value = fRand(-100000, 100000);
    Runtime::GetConstantPool()->SetNum(constant_pool_id, value);
}

void RuntimeImage::CreateRandStrInConstantPool(uint8_t constant_pool_id) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    size_t string_length = rand() % 100 + 1;
    ASSERT(string_length + metainfo_constant_pool_.size >= SIZE_MATAINFO_CONST_POOL);
    auto metainfo_string = metainfo_constant_pool_.data.data();
    auto start_string = metainfo_constant_pool_.size;
    for (size_t i = 0 ; i < string_length; i++) {
        metainfo_string[start_string + 1] = rand() % 127 + 1;
    }
    metainfo_string[start_string + string_length] = '\0';
    metainfo_constant_pool_.size += string_length + 1;

    Runtime::GetConstantPool()->SetStr(constant_pool_id, start_string);
}


void RuntimeImage::CreateStrInConstantPool(uint8_t constant_pool_id, const char *str) {
    ASSERT(!const_pool_image_[constant_pool_id]);
    const_pool_image_[constant_pool_id] = true;

    uint64_t string_start = reinterpret_cast<uint64_t>(metainfo_constant_pool_.data.data() + metainfo_constant_pool_.size);
    size_t string_length = strlen(str);
    ASSERT(string_length + metainfo_constant_pool_.size >= SIZE_MATAINFO_CONST_POOL);

    strcpy(metainfo_constant_pool_.data.data(), str);
    metainfo_constant_pool_.size += string_length + 1;

    Runtime::GetConstantPool()->SetStr(constant_pool_id, string_start);
}

}

}
