#include "../support/noncopyable.hpp"

template <typename T>
class ThreadLocalPtr : private wheels::NonCopyable {
public:
    ThreadLocalPtr() {
        Init();
    }

    operator T*() {
        return LoadTypedPointer();
    }

    ThreadLocalPtr& operator=(T* ptr) {
        StorePointer(ptr);
        return *this;
    }

    T* Exchange(T* ptr) {
        T* old_ptr = LoadTypedPointer();
        StorePointer(ptr);
        return old_ptr;
    }

    // Usage: thread_local_ptr->Method();
    T* operator->() {
        return LoadTypedPointer();
    }

    explicit operator bool() const {
        return LoadTypedPointer() != nullptr;
    }

private:
    void Init() {
        auto ctor = []() {
            return nullptr;
        };
        auto dtor = [](void* /*raw_ptr*/) {
            // Nop
        };
        slot_index_ = TLSManager::Instance().AcquireSlot(ctor, dtor);
    }

    T* LoadTypedPointer() {
        void** slot = AccessSlot();
        void* raw_ptr = *slot;
        return (T*)raw_ptr;
    }

    void StorePointer(T* ptr) {
        void** slot = AccessSlot();
        *slot = ptr;
    }

    void** AccessSlot() {
        void*& slot = TLSManager::Instance().Access(slot_index_);
        return &slot;
    }

private:
    size_t slot_index_;
};
