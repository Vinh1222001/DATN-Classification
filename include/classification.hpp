// #pragma once
// #ifndef CLASSIFICATION_HPP
// #define CLASSIFICATION_HPP

// #include "base_module.hpp"
// #include "types.hpp"
// #include "camera.hpp"
// #include <vector>

// struct ResultBoundingBox
// {
//     const char *label;
//     uint32_t x;
//     uint32_t y;
//     uint32_t width;
//     uint32_t height;
//     float value;
// };

// using BoundingBox = Types::SemaphoreMutexData<std::vector<ResultBoundingBox>>;

// class Classification : public BaseModule
// {
// private:
//     bool debugNn;
//     BoundingBox bb;
//     Camera *camera = nullptr;
//     void taskFn() override;
//     void classify();

// public:
//     Classification(Camera *cam, bool debugNn = false);
//     ~Classification();

//     std::vector<ResultBoundingBox> getClassifyResult();
// };

// #endif
