/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include <limits>

#include "metatype_fwd.h"
#include "extensions/allocation.h"
#include "extensions/streams.h"
#include "extensions/name.h"

namespace N {

template<class Extension> TypeId Extensions::Ex<Extension>::typeId()
{
    return qTypeId<Extension, Name_hash>();
}

namespace P {

template<class... Extensions>
void TypeIdData::registerExtensions(Extensions... extensions)
{
    (registerExtension(Extensions::typeId(), extensions), ...);
}

void TypeIdData::registerExtension(TypeId extensionId, Extensions::ExtensionBase extension)
{
    knownExtensions.try_emplace(extensionId, extension);
}

}  // namespace P


/*!
 * Trick to have always at most one type id per type.
 *
 * When we call the function the first time the proposed newId would be
 * set as the one used across the application. On the second call the newId would be ignored. It
 * is up to called to figure-out, which case it was.
 */
template<class T>
TypeId qTypeIdImpl(TypeId newId)
{
    static TypeId id = newId;
    return id;
}

/*!
 * The function to get the type id, more or less equivalent of qMetaTypeId.
 *
 * The function creates or gets id for a given type. It also can register Extensions
 * if there are new given.
 *
 * If called without Extensions, then a default set of Extensions is used.
 *
 * TODO The id is not cached as in qRegisterMetaType, that could be a problem, but not
 * necessarily as we do recommend to call qRegisterMetaType only once. So it should not
 * be in a hot code path and the cost is minor.
 * TODO Maybe a bit expensive if many qTypeId<T> with different extensions are used, because
 * static typeData possibly would be initialized but unused.
 * TODO We may consider change qTypeIdImpl to hold an atomic TypeId and check for nullptr
 * to see if a type already has an id or not, something to measure, as atomic access would
 * likely be more expensive then static access.
 * TODO think about lifetime of Extensions in context of plugins
 */
template<class T, class... Extensions>
TypeId qTypeId()
{
    if constexpr (!bool(sizeof...(Extensions))) {
        // Register default stuff, Qt should define minimal useful set, DataStream is probably not in :-)
        // Every usage of metatype can call qTypeId with own minimal set of extensions.
#ifndef Q_CC_MSVC
        return qTypeId<T, N::Extensions::Allocation, N::Extensions::DataStream, N::Extensions::Name_dlsym, N::Extensions::Name_hash>();
#else // msvc
        return qTypeId<T, N::Extensions::Allocation, N::Extensions::DataStream, N::Extensions::Name_hash>();
#endif
    }
    (Extensions::template PreRegisterAction<T>(), ...);
    using ExtendedTypeIdData = N::P::TypeIdDataExtended<sizeof...(Extensions) + 1>;
    static ExtendedTypeIdData typeData{{sizeof...(Extensions) + 1, {}},
                                       {{Extensions::typeId(), Extensions::template createExtension<T>()}...}};
    auto id = qTypeIdImpl<T>(&typeData);
    if (id != &typeData) {
        // We are adding extensions
        // TODO try to re-use typeData, maybe we should just link them?
        id->registerExtensions(Extensions::template createExtension<T>()...);
    }
    (Extensions::template PostRegisterAction<T>(id), ...);
    return id;
}

template<class Extension>
inline void Extensions::Ex<Extension>::Call(TypeId id, quint8 operation, size_t argc, void **argv)
{
    if (!id->call(typeId(), operation, argc, argv)) {
        // TODO Think when we need the warning, sometimes we want just to probe if it is possible to do stuff
#ifndef Q_CC_MSVC
        warnAboutFailedCall(qTypeId<Extension, Name_dlsym>(), id);
#else // msvc
        // warnAboutFailedCall(qTypeId<Extension, Name_function_hack>(), id);
#endif
    }
}

}  // namespace N
