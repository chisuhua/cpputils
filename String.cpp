#include "String.h"

StringPtr StringPool::intern(const char* str, std::uint32_t hash) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pool_.find(hash);
    if (it != pool_.end()) {
        return it->second.lock();
    }
    auto deleter = [this, hash](const String* p) {
        std::lock_guard<std::mutex> lock(this->mutex_);
        //this->remove(hash);
        this->pool_.erase(hash);
        delete p;
    };
    auto string_ref = StringPtr(new String(str), deleter);
    pool_.emplace(hash, std::weak_ptr<const String>(string_ref));
    return string_ref;
};

StringRef StringNull{""_hs};
