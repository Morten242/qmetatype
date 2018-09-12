#pragma once

#include <QtCore>

namespace N {

namespace P {


typedef void* (*QtMetTypeCall)(size_t functionType, size_t argc, void **argv);
template<class T> void* metaTypeCallImpl(size_t functionType, size_t argc, void **argv);

}  // namespace P

using TypeId = P::QtMetTypeCall;

template<class T> TypeId qRegisterType();

}  // namespace N
