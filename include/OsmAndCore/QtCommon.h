#ifndef _OSMAND_CORE_QT_COMMON_H_
#define _OSMAND_CORE_QT_COMMON_H_

#include <cassert>
#include <memory>
#include <iostream>
#include <algorithm>
#include <functional>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QMap>
#include <QList>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

namespace OsmAnd
{
    template<typename KEY, typename VALUE>
    Q_DECL_CONSTEXPR QMutableHashIterator<KEY, VALUE> mutableIteratorOf(QHash<KEY, VALUE>& container)
    {
        return QMutableHashIterator<KEY, VALUE>(container);
    }

    template<typename KEY, typename VALUE>
    Q_DECL_CONSTEXPR QMutableMapIterator<KEY, VALUE> mutableIteratorOf(QMap<KEY, VALUE>& container)
    {
        return QMutableMapIterator<KEY, VALUE>(container);
    }

    template<typename T>
    Q_DECL_CONSTEXPR QMutableListIterator<T> mutableIteratorOf(QList<T>& container)
    {
        return QMutableListIterator<T>(container);
    }

    template<typename T>
    Q_DECL_CONSTEXPR QMutableVectorIterator<T> mutableIteratorOf(QVector<T>& container)
    {
        return QMutableVectorIterator<T>(container);
    }

    template<typename KEY, typename VALUE>
    Q_DECL_CONSTEXPR QHashIterator<KEY, VALUE> iteratorOf(const QHash<KEY, VALUE>& container)
    {
        return QHashIterator<KEY, VALUE>(container);
    }

    template<typename KEY, typename VALUE>
    Q_DECL_CONSTEXPR QMapIterator<KEY, VALUE> iteratorOf(const QMap<KEY, VALUE>& container)
    {
        return QMapIterator<KEY, VALUE>(container);
    }

    template<typename T>
    Q_DECL_CONSTEXPR QListIterator<T> iteratorOf(const QList<T>& container)
    {
        return QListIterator<T>(container);
    }

    template<typename T>
    Q_DECL_CONSTEXPR QVectorIterator<T> iteratorOf(const QVector<T>& container)
    {
        return QVectorIterator<T>(container);
    }

    template<typename KEY, typename VALUE>
    QHash< KEY, VALUE > hashFrom(const QList<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QHash< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename KEY, typename VALUE>
    QHash< KEY, VALUE > hashFrom(const QVector<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QHash< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename T>
    auto detachedOf(const T& input) -> typename std::enable_if<std::is_same<decltype(input.detach()), void()>::value && !std::is_same<decltype(std::begin(input)), void()>::value, T>::type
    {
        auto copy = input;
        copy.detach();
        for (auto& value : copy)
            value = detachedOf(value);
        return copy;
    }

    template<typename T>
    auto detachedOf(const T& input) -> typename std::enable_if< std::is_same<decltype(input.detach()), void()>::value, T>::type
    {
        auto copy = input;
        copy.detach();
        return copy;
    }

    template<typename T>
    Q_DECL_CONSTEXPR T detachedOf(const T& input)
    {
        return input;
    }
}

#endif // !defined(_OSMAND_CORE_QT_COMMON_H_)