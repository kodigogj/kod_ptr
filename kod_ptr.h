#pragma once
#include <type_traits>

namespace kod {
struct kod_ptr_base {
protected:
    struct data {
        void* pointer;
        inline data(void* const& pointer)
            : pointer{ pointer }
        {
        }
        inline virtual void destroy() = 0;
    };
    union counter {
        unsigned __int16 counts[2]{ 0 }; // shared, weak
        unsigned __int32 count;
    };
    template <typename T, bool /*is_array*/>
    struct default_data : data {
        inline default_data(void* const& pointer)
            : data{ pointer }
        {
        }
        inline void destroy() override
        {
            delete static_cast<T*>(pointer);
            pointer = nullptr;
        }
    };
    template <typename T>
    struct default_data<T, true> : data {
        inline default_data(void* const& pointer)
            : data{ pointer }
        {
        }
        inline void destroy() override
        {
            delete[] static_cast<T*>(pointer);
            pointer = nullptr;
        }
    };
    //-------
    inline virtual data* const& get_data() const = 0;
    inline virtual counter* const& get_counter() const = 0;
};

template <typename T, bool weak = false>
struct kod_ptr;
template <typename T>
struct kod_ptr<T, false> : private kod_ptr_base {
private:
    template <typename From, typename To>
    using is_convertible = std::is_convertible<std::decay_t<std::remove_extent_t<From>>*, std::decay_t<std::remove_extent_t<To>>*>;
    template <typename U, typename V>
    using is_comparable = std::is_same<decltype(std::declval<std::decay_t<std::remove_extent_t<U>>*>() == std::declval<std::decay_t<std::remove_extent_t<V>>*>()), bool>;
    template <typename U, typename V = std::decay_t<std::remove_extent_t<U>>>
    using data = default_data<V, std::is_array_v<U>>;
    using type = std::decay_t<std::remove_extent_t<T>>;
    //-------
    kod_ptr_base::data* mdata{ nullptr };
    counter* mcount{ nullptr };
    //-------
public:
    inline kod_ptr() {}
    inline kod_ptr(std::nullptr_t) {}
    inline kod_ptr(const kod_ptr& arg)
        : mdata{ arg.mdata }
        , mcount{ arg.mcount }
    {
        if (mcount) {
            ++mcount->counts[0];
        }
    }
    template <typename K, bool shared, std::enable_if_t<is_convertible<K, T>::value, int> = 0>
    inline kod_ptr(const kod_ptr<K, shared>& arg)
        : mdata{ arg.mdata }
        , mcount{ arg.mcount }
    {
        if (mcount) {
            ++mcount->counts[0];
        }
    }
    template <typename K, std::enable_if_t<std::conjunction_v<is_convertible<K, T>, std::is_base_of<kod_ptr_base, K>>, int> = 0>
    inline kod_ptr(K*&& arg)
        : mdata{ arg->get_data() }
        , mcount{ arg->get_counter() }
    {
        if (mcount) {
            ++mcount->counts[0];
        }
    }
    template <typename K, std::enable_if_t<std::conjunction_v<is_convertible<K, T>, std::negation<std::is_base_of<kod_ptr_base, K>>>, int> = 0>
    inline kod_ptr(K*&& arg)
        : mdata{ arg ? new data<K>{ arg } : nullptr }
        , mcount{ arg ? new counter{ 1, 0 } : nullptr }
    {
    }
    //-------
    inline constexpr kod_ptr& operator=(std::nullptr_t)
    {
        this->~kod_ptr();
        mdata = nullptr;
        mcount = nullptr;
        return *this;
    }
    inline constexpr kod_ptr& operator=(const kod_ptr& arg)
    {
        if (mdata == arg.mdata)
            return *this;
        this->~kod_ptr();
        mdata = arg.mdata;
        mcount = arg.mcount;
        if (mcount) {
            ++mcount->counts[0];
        }
        return *this;
    }
    template <typename K, bool shared, std::enable_if_t<is_convertible<K, T>::value, int> = 0>
    inline constexpr kod_ptr& operator=(const kod_ptr<K, shared>& arg)
    {
        if (mdata == arg.mdata)
            return *this;
        this->~kod_ptr();
        mdata = arg.mdata;
        mcount = arg.mcount;
        if (mcount) {
            ++mcount->counts[0];
        }
        return *this;
    }
    template <typename K, std::enable_if_t<std::conjunction_v<is_convertible<K, T>, std::is_base_of<kod_ptr_base, K>>, int> = 0>
    inline constexpr kod_ptr& operator=(K*&& arg)
    {
        if (arg && mdata == arg->get_data())
            return *this;
        this->~kod_ptr();
        mdata = arg->get_data();
        mcount = arg->get_counter();
        if (mcount) {
            ++mcount->counts[0];
        }
        return *this;
    }
    template <typename K, std::enable_if_t<std::conjunction_v<is_convertible<K, T>, std::negation<std::is_base_of<kod_ptr_base, K>>>, int> = 0>
    inline constexpr kod_ptr& operator=(K*&& arg)
    {
        if (mdata && mdata->pointer == arg)
            return *this;
        this->~kod_ptr();
        mdata = arg ? new data<K>{ arg } : nullptr;
        mcount = arg ? new counter{ 1, 0 } : nullptr;
        return *this;
    }
    //-------
private:
    inline kod_ptr_base::data* const& get_data() const { return mdata; }
    inline counter* const& get_counter() const { return mcount; };
    //-------
public:
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator==(const kod_ptr<K>& arg) const
    {
        return (mdata ? mdata->pointer : nullptr) == arg.mdata->pointer;
    }
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator!=(const kod_ptr<K>& arg) const
    {
        return (mdata ? mdata->pointer : nullptr) != arg.mdata->pointer;
    }
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator==(K* const& ptr) const
    {
        return (mdata ? mdata->pointer : nullptr) == ptr;
    }
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator!=(K* const& ptr) const
    {
        return (mdata ? mdata->pointer : nullptr) != ptr;
    }
    //-------
    inline constexpr type& operator*() const
    {
        return *static_cast<type*>(mdata ? mdata->pointer : nullptr);
    }
    inline constexpr type* const& operator->() const
    {
        return static_cast<type*>(mdata ? mdata->pointer : nullptr);
    }
    inline constexpr type& operator[](size_t i) const
    {
        return static_cast<type*>(mdata ? mdata->pointer : nullptr)[i];
    }
    inline constexpr operator bool() const
    {
        return mdata ? mdata->pointer : false;
    }

    inline constexpr explicit operator type* const&()
    {
        return static_cast<type*>(mdata ? mdata->pointer : nullptr);
    }
    //-------
    inline ~kod_ptr()
    {
        if (mcount) {
            if (--mcount->counts[0] == 0) {
                mdata->destroy();
            }
            if (mcount->count == 0) {
                delete mcount;
                delete mdata;
            }
        }
    }
};
template <typename T>
struct kod_ptr<T, true> : private kod_ptr_base {
private:
    template <typename From, typename To>
    using is_convertible = std::is_convertible<std::decay_t<std::remove_extent_t<From>>*, std::decay_t<std::remove_extent_t<To>>*>;
    template <typename U, typename V>
    using is_comparable = std::is_same<decltype(std::declval<std::decay_t<std::remove_extent_t<U>>*>() == std::declval<std::decay_t<std::remove_extent_t<V>>*>()), bool>;
    template <typename U, typename V = std::decay_t<std::remove_extent_t<U>>>
    using data = default_data<V, std::is_array_v<U>>;
    using type = std::decay_t<std::remove_extent_t<T>>;
    //-------
    kod_ptr_base::data* mdata{ nullptr };
    counter* mcount{ nullptr };
    //-------
public:
    inline kod_ptr() {}
    inline kod_ptr(std::nullptr_t) {}
    inline kod_ptr(const kod_ptr& arg)
        : mdata{ arg.mdata }
        , mcount{ arg.mcount }
    {
        if (mcount) {
            ++mcount->counts[1];
        }
    }
    template <typename K, bool shared, std::enable_if_t<is_convertible<K, T>::value, int> = 0>
    inline kod_ptr(const kod_ptr<K, shared>& arg)
        : mdata{ arg.mdata }
        , mcount{ arg.mcount }
    {
        if (mcount) {
            ++mcount->counts[1];
        }
    }
    template <typename K, std::enable_if_t<std::conjunction_v<is_convertible<K, T>, std::is_base_of<kod_ptr_base, K>>, int> = 0>
    inline kod_ptr(K*&& arg)
        : mdata{ arg->get_data() }
        , mcount{ arg->get_counter() }
    {
        if (mcount) {
            ++mcount->counts[1];
        }
    }
    //-------
    inline constexpr kod_ptr& operator=(std::nullptr_t)
    {
        this->~kod_ptr();
        mdata = nullptr;
        mcount = nullptr;
        return *this;
    }
    inline constexpr kod_ptr& operator=(const kod_ptr& arg)
    {
        if (mdata == arg.mdata)
            return *this;
        this->~kod_ptr();
        mdata = arg.mdata;
        mcount = arg.mcount;
        if (mcount) {
            ++mcount->counts[1];
        }
        return *this;
    }
    template <typename K, bool shared, std::enable_if_t<is_convertible<K, T>::value, int> = 0>
    inline constexpr kod_ptr& operator=(const kod_ptr<K, shared>& arg)
    {
        if (mdata == arg.mdata)
            return *this;
        this->~kod_ptr();
        mdata = arg.mdata;
        mcount = arg.mcount;
        if (mcount) {
            ++mcount->counts[1];
        }
        return *this;
    }
    template <typename K, std::enable_if_t<std::conjunction_v<is_convertible<K, T>, std::is_base_of<kod_ptr_base, K>>, int> = 0>
    inline constexpr kod_ptr& operator=(K*&& arg)
    {
        if (arg && mdata == arg->get_data())
            return *this;
        this->~kod_ptr();
        mdata = arg->get_data();
        mcount = arg->get_counter();
        if (mcount) {
            ++mcount->counts[1];
        }
        return *this;
    }
    //-------
private:
    inline kod_ptr_base::data* const& get_data() const { return mdata; }
    inline counter* const& get_counter() const { return mcount; };
    //-------
public:
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator==(const kod_ptr<K>& arg) const
    {
        return (mdata ? mdata->pointer : nullptr) == arg.mdata->pointer;
    }
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator!=(const kod_ptr<K>& arg) const
    {
        return (mdata ? mdata->pointer : nullptr) != arg.mdata->pointer;
    }
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator==(K* const& ptr) const
    {
        return (mdata ? mdata->pointer : nullptr) == ptr;
    }
    template <typename K, std::enable_if_t<is_comparable<T, K>::value, int> = 0>
    inline constexpr bool operator!=(K* const& ptr) const
    {
        return (mdata ? mdata->pointer : nullptr) != ptr;
    }
    //-------
    inline constexpr type& operator*() const
    {
        return *static_cast<type*>(mdata ? mdata->pointer : nullptr);
    }
    inline constexpr type* const& operator->() const
    {
        return static_cast<type*>(mdata ? mdata->pointer : nullptr);
    }
    inline constexpr type& operator[](size_t i) const
    {
        return static_cast<type*>(mdata ? mdata->pointer : nullptr)[i];
    }
    inline constexpr operator bool() const
    {
        return mdata ? mdata->pointer : false;
    }

    inline constexpr explicit operator type* const&()
    {
        return static_cast<type*>(mdata ? mdata->pointer : nullptr);
    }
    //-------
    inline ~kod_ptr()
    {
        if (mcount) {
            --mcount->counts[1];
            if (mcount->count == 0) {
                delete mcount;
                delete mdata;
            }
        }
    }
};
}