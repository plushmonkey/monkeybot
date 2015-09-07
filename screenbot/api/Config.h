#ifndef API_CONFIG_H_
#define API_CONFIG_H_

namespace api {

template <class T>
using StorageType = typename std::decay<T>::type;

struct Any {
    bool IsNull() const { return !m_Ptr; }
    bool NotNull() const { return m_Ptr != nullptr; }

    template<typename U> Any(U&& value)
        : m_Ptr(new Derived<StorageType<U>>(std::forward<U>(value)))
    {

    }

    template<class U>
    bool Is() const {
        typedef StorageType<U> T;

        auto derived = dynamic_cast<Derived<T>*>(m_Ptr);

        return derived != nullptr;
    }

    template<class U>
    StorageType<U>& As() {
        typedef StorageType<U> T;

        auto derived = dynamic_cast<Derived<T>*> (m_Ptr);

        if (!derived)
            throw std::bad_cast();

        return derived->value;
    }

    template<class U>
    operator U() {
        return As<StorageType<U>>();
    }

    Any() : m_Ptr(nullptr) { }

    Any(Any& that) : m_Ptr(that.Clone()) { }

    Any(Any&& that) : m_Ptr(that.m_Ptr) {
        that.m_Ptr = nullptr;
    }

    Any& operator=(const Any& a) {
        if (m_Ptr == a.m_Ptr)
            return *this;

        auto old_ptr = m_Ptr;

        m_Ptr = a.Clone();

        if (old_ptr)
            delete old_ptr;

        return *this;
    }

    Any& operator=(Any&& a) {
        if (m_Ptr == a.m_Ptr)
            return *this;

        std::swap(m_Ptr, a.m_Ptr);

        return *this;
    }

    ~Any() {
        delete m_Ptr;
    }

private:
    struct Base {
        virtual ~Base() {}

        virtual Base* Clone() const = 0;
    };

    template<typename T>
    struct Derived : Base {
        template<typename U> Derived(U&& value) : value(std::forward<U>(value)) { }

        T value;

        Base* Clone() const { return new Derived<T>(value); }
    };

    Base* Clone() const {
        if (m_Ptr)
            return m_Ptr->Clone();
        else
            return nullptr;
    }

    Base* m_Ptr;
};

} // ns api

#endif
