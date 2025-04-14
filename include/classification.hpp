#pragma once
#ifndef CLASSIFICATION_HPP
#define CLASSIFICATION_HPP

#include "global.hpp"
#include <vector>

using BoundingBox = Types::SemaphoreMutexData<std::vector<ei_impulse_result_bounding_box_t>>;

class Classification : public BaseModule
{
private:
    bool debugNn;
    BoundingBox bb;
    Camera *camera = nullptr;
    void taskFn() override;
    void classify();

public:
    Classification(Camera *cam, bool debugNn = false);
    ~Classification();

    std::vector<ei_impulse_result_bounding_box_t> getClassifyResult();
};

#endif
