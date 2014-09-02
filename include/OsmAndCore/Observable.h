#ifndef _OSMAND_CORE_OBSERVABLE_H_
#define _OSMAND_CORE_OBSERVABLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QRunnable>
#include <OsmAndCore/QtCommon.h>

#include <OsmAndCore.h>

namespace OsmAnd
{

#if !defined(SWIG)
    template<typename... ARGS>
    class Observable
    {
        Q_DISABLE_COPY_AND_MOVE(Observable);
    public:
        typedef std::function<void (ARGS...)> Handler;
        typedef const void* Tag;

    private:
        mutable QReadWriteLock _observersLock;
        mutable QHash<Tag, Handler> _observers;

        struct NotifyRunnable : public QRunnable
        {
            Q_DISABLE_COPY_AND_MOVE(NotifyRunnable);

            typedef std::function<void()> NotifyFunctor;
            const NotifyFunctor notifyFunctor;

            NotifyRunnable(const NotifyFunctor notifyFunctor_)
                : notifyFunctor(notifyFunctor_)
            {
            }

            virtual ~NotifyRunnable()
            {
            }

            virtual void run()
            {
                notifyFunctor();
            }
        };
    public:
        Observable()
        {
        }
        ~Observable()
        {
        }

        bool attach(const Tag tag, const Handler handler) const
        {
            QWriteLocker scopedLocker(&_observersLock);

            if (_observers.contains(tag) || handler == nullptr)
                return false;

            _observers.insert(tag, handler);

            return true;
        }

        bool detach(const Tag tag) const
        {
            QWriteLocker scopedLocker(&_observersLock);

            return (_observers.remove(tag) > 0);
        }

        void notify(ARGS... args) const
        {
            QHash<Tag, Handler> observers;
            {
                QReadLocker scopedLocker(&_observersLock);
                observers = detachedOf(_observers);
            }

            for(const auto& handler : constOf(observers))
                handler(args...);
        }

        void postNotify(ARGS... args) const
        {
            QHash<Tag, Handler> observers;
            {
                QReadLocker scopedLocker(&_observersLock);
                observers = detachedOf(_observers);
            }

            QThreadPool::globalInstance()->start(new NotifyRunnable(
                [=]
                ()
                {
                    for(const auto& handler : constOf(observers))
                        handler(args...);
                }));
        }
    };

    template<typename _>
    class ObservableAs;

    // ObservableAs for function pointer
    template<typename RETURN_TYPE, typename... ARGS>
    class ObservableAs<RETURN_TYPE(*)(ARGS...)> : public Observable< ARGS... >
    {
        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable< ARGS... >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        ~ObservableAs()
        {
        }
    };

    // ObservableAs for member function pointer
    template<typename CLASS, typename RETURN_TYPE, typename... ARGS>
    class ObservableAs<RETURN_TYPE(CLASS::*)(ARGS...)> : public Observable< ARGS... >
    {
        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable< ARGS... >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        ~ObservableAs()
        {
        }
    };

    // ObservableAs for const member function pointer
    template<typename CLASS, typename RETURN_TYPE, typename... ARGS>
    class ObservableAs<RETURN_TYPE(CLASS::*)(ARGS...) const> : public Observable< ARGS... >
    {
        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable< ARGS... >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        ~ObservableAs()
        {
        }
    };

    // ObservableAs for member object pointer
    template<typename CLASS, typename RETURN_TYPE>
    class ObservableAs<RETURN_TYPE(CLASS::*)> : public Observable<>
    {
        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename Observable<>::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        ~ObservableAs()
        {
        }
    };

    // ObservableAs for functor
    template<class F>
    class ObservableAs : public ObservableAs< decltype(&F::operator()) >
    {
        Q_DISABLE_COPY_AND_MOVE(ObservableAs);
    public:
        typedef typename ObservableAs< decltype(&F::operator()) >::Tag Tag;

    private:
    protected:
    public:
        ObservableAs()
        {
        }
        ~ObservableAs()
        {
        }
    };

#else

    // Just a stub for SWIG
    template<typename T>
    struct ObservableAs
    {
        typedef const void* Tag;

        ObservableAs();
        ~ObservableAs();

        bool detach(const Tag tag) const;
    };

#endif

}

#endif // !defined(_OSMAND_CORE_OBSERVABLE_H_)