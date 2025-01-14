#ifndef STRING_INTERN_H
#define STRING_INTERN_H

#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <mutex>
#include <unordered_map>
#include <string_view>
#include <cstddef>
#include <cassert>
#include <atomic>

// FNV-1a hash function
constexpr inline uint32_t fnv1a(const char* str, std::size_t length) {
    uint32_t hash = 0x811c9dc5;
    for (std::size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint32_t>(str[i]);
        hash *= 0x01000193;
    }
    return hash;
}

inline uint32_t fnv1a_runtime(const char* str, std::size_t length) {
    uint32_t hash = 0x811c9dc5;
    for (std::size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint32_t>(str[i]);
        hash *= 0x01000193;
    }
    return hash;
}

class StringPool;
class String;
using StringPtr = std::shared_ptr<const String>;

template<typename T>
struct is_allowed_string_type :
    std::disjunction<
        std::is_same<std::decay_t<T>, std::string>,
        std::is_same<std::decay_t<T>, char*>,
        std::is_same<std::decay_t<T>, const char*>
    > {};

// 特化处理 const char[N]
template<std::size_t N>
struct is_allowed_string_type<const char[N]> : std::true_type {};

// 特化处理 char[N]
template<std::size_t N>
struct is_allowed_string_type<char[N]> : std::true_type {};

class String {
public:
    std::string data;

    size_t length() const { return data.length(); }
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    char operator[](size_t index) const { return data[index]; }
    char at(size_t index) const { return data.at(index); }
    size_t find(const std::string& str, size_t pos = 0) const { return data.find(str, pos); }
    size_t rfind(const std::string& str, size_t pos = std::string::npos) const { return data.rfind(str, pos); }
    std::string substr(size_t pos = 0, size_t len = std::string::npos) const { return data.substr(pos, len); }

    bool operator==(const String& other) const { return data == other.data; }
    bool operator!=(const String& other) const { return data != other.data; }

private:
    // No copy and assignment constructors
    String(const String&) = delete;
    String& operator=(const String&) = delete;
    explicit String(const char* str) : data(str) {}
    explicit String(const std::string& str) : data(str) {}

public:
    friend class StringPool;
};

class StringPool {
public:
    static std::shared_ptr<StringPool> getInstance() {
        static std::shared_ptr<StringPool> instance(new StringPool());
        return instance;
    }

    template<typename T, typename = std::enable_if_t<is_allowed_string_type<T>::value>>
    static StringPtr try_emplace(T&& arg) {
        std::string str(arg);
        return StringPool::getInstance()->intern(str.c_str(), fnv1a(str.c_str(), str.length()));
    }

    StringPtr intern(const char* str, std::uint32_t hash);

    ~StringPool() {
        clearPool();
    }

    bool isStringIntern(const std::string& str) {
        return getStringByHash(fnv1a_runtime(str.c_str(), str.length())) != nullptr;
    }

    StringPtr getStringByHash(std::uint32_t hash) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pool_.find(hash);
        if (it != pool_.end()) {
            return it->second.lock();
        }
        return nullptr;
    }

private:
    StringPool() = default;

    void clearPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            auto it = pool_.begin();
            pool_.erase(it); // Erase from the pool map
        }
    }

    std::mutex mutex_;
    std::unordered_map<std::uint32_t, std::weak_ptr<const String>> pool_;
};

class StringRef {
public:
    template<typename T, typename = std::enable_if_t<is_allowed_string_type<T>::value>>
    StringRef(T&& arg) : ptr_(StringPool::try_emplace(std::forward<T>(arg))) {}

    StringRef(StringPtr ptr) : ptr_(ptr) {}
    //StringRef(StringPtr& ptr) : ptr_(ptr) {}

    StringRef(const StringRef& other) : ptr_(other.ptr_) {};
    StringRef& operator=(const StringRef& other) {
        ptr_ = other.ptr_;
        return *this;
    }

    StringRef(StringRef&& other) noexcept : ptr_(std::move(other.ptr_)) {}
    StringRef& operator=(StringRef&& other) noexcept {
        if (this != &other) {
            ptr_ = std::move(other.ptr_);
        }
        return *this;
    }

    // Get the shared pointer
    operator StringPtr() const {
        return ptr_;
    }

    const String* operator->() const {
        return ptr_.get();
    }

    //const String& operator.() const {
        //return *ptr_;
    //}

    const String& operator*() const {
        return *ptr_;
    }

    bool operator==(const StringRef& other) const {
        return ptr_.get() == other.ptr_.get();
    }

    bool operator!=(const StringRef& other) const {
        return ptr_.get() != other.ptr_.get();
    }

    const String* getRawPointer() const {
        return ptr_.get();
    }

    size_t length() const { return ptr_->length(); }
    size_t size() const { return ptr_->size(); }
    bool empty() const { return ptr_->empty(); }
    char operator[](size_t index) const { return (*ptr_)[index]; }
    char at(size_t index) const { return ptr_->at(index); }
    size_t find(const std::string& str, size_t pos = 0) const { return ptr_->find(str, pos); }
    size_t rfind(const std::string& str, size_t pos = std::string::npos) const { return ptr_->rfind(str, pos); }
    std::string substr(size_t pos = 0, size_t len = std::string::npos) const { return ptr_->substr(pos, len); }

private:
    StringPtr ptr_;
};

// Custom hash function for StringRef
namespace std {
    template<>
    struct hash<StringRef> {
        size_t operator()(const StringRef& ref) const {
            return std::hash<const String*>{}(static_cast<StringPtr>(ref).get());
        }
    };
}

inline StringPtr operator"" _hs(const char* str, std::size_t length) {
    uint32_t hash = fnv1a(str, length);
    return StringPool::getInstance()->intern(str, hash);
}

inline StringPtr StringPool::intern(const char* str, std::uint32_t hash) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pool_.find(hash);
    if (it != pool_.end()) {
        return it->second.lock();
    }
    auto deleter = [this, hash](const String* p) {
        std::lock_guard<std::mutex> lock(this->mutex_);
        this->pool_.erase(hash);
        delete p;
    };
    auto string_ref = StringPtr(new String(str), deleter);
    pool_.emplace(hash, std::weak_ptr<const String>(string_ref));
    return string_ref;
}

#endif // STRING_INTERN_H

