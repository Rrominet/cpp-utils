#pragma once
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>
#include <any>
#include "./Ret.h"

namespace ml
{
    struct Slot
    {
        std::any object;
        unsigned int generation = 1;
    };

    class ObjectsManager;

    template<typename T>
        struct Handle
        {
            public:
                Handle() = default;

                /*
                 * Allows:
                 * Handle<Child> -> Handle<Parent>
                 * Only when Child* can safely and implicitly convert to Parent*.
                 */
                template<
                    typename U,
                             std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0
                                 >
                                 Handle(const Handle<U>& other)
                                 : id(other.id),
                                 generation(other.generation),
                                 _manager(other._manager),
                                 _pointer(static_cast<T*>(other._pointer))
            {
            }

                unsigned int id = std::numeric_limits<unsigned int>::max();
                unsigned int generation = 0;

                bool valid() const
                {
                    return _manager != nullptr
                        && _pointer != nullptr
                        && id != std::numeric_limits<unsigned int>::max()
                        && generation != 0;
                }

                explicit operator bool() const
                {
                    return valid();
                }

                T* get();
                const T* get() const;

            private:
                Handle(
                        ObjectsManager* manager,
                        unsigned int objectId,
                        unsigned int objectGeneration,
                        T* pointer
                      )
                    : id(objectId),
                    generation(objectGeneration),
                    _manager(manager),
                    _pointer(pointer)
            {
            }

                ObjectsManager* _manager = nullptr;

                // Non-owning typed view of the object owned by the Slot.
                T* _pointer = nullptr;

                /*
                 * Allows Handle<Parent> to access the private members of
                 * Handle<Child> during conversion.
                 */
                template<typename>
                    friend struct Handle;

                friend class ObjectsManager;
        };

    class ObjectsManager
    {
        private : 
            std::vector<Slot> _slots; 
            std::vector<unsigned int> _free;

            template<typename S>
                bool _checkHandle(const Handle<S> handle) const
                {
                    if (!handle.valid())
                    {
                        lg("Handle " << handle.id << " is not valid.");
                        return false;
                    }

                    if (handle._manager != this)
                    {
                        lg("Handle " << handle.id
                                << " belongs to another ObjectsManager.");
                        return false;
                    }

                    if (handle.id >= _slots.size())
                    {
                        lg("Handle " << handle.id
                                << " is out of range. ("
                                << _slots.size()
                                << " slots)");

                        return false;
                    }

                    return true;
                }

            bool _checkSlot(const Slot& slot, unsigned int generation) const;

        public : 
            template <typename S, typename ... Args>
                Handle<S> create(Args&& ... args)
                {
                    unsigned int id;

                    if (_free.empty())
                    {
                        id = static_cast<unsigned int>(_slots.size());
                        _slots.emplace_back();
                    }
                    else
                    {
                        id = _free.back();
                        _free.pop_back();
                    }

                    Slot& slot = _slots.at(id);

                    auto object = std::make_shared<S>(
                            std::forward<Args>(args)...
                            );

                    S* pointer = object.get();

                    // std::any still owns the concrete shared_ptr<S>.
                    slot.object = std::move(object);

                    return Handle<S>(
                            this,
                            id,
                            slot.generation,
                            pointer
                            );
                } 

            template<typename S>
                S* get(const Handle<S>& handle)
                {
                    if (!_checkHandle(handle))
                        return nullptr;

                    Slot& slot = _slots[handle.id];

                    if (!_checkSlot(slot, handle.generation))
                        return nullptr;

                    return handle._pointer;
                }

            template<typename S>
                const S* get(const Handle<S>& handle) const
                {
                    if (!_checkHandle(handle))
                        return nullptr;

                    const Slot& slot = _slots[handle.id];

                    if (!_checkSlot(slot, handle.generation))
                        return nullptr;

                    return handle._pointer;
                }

            template <typename S>
                ml::Ret<> destroy(const Handle<S>& handle)
                {
                    if (!handle.valid()) 
                        return ml::ret::fail("The handle is non-valid - can't destroy the object assiociated to it.");
                    if (handle.id >= _slots.size())
                        return ml::ret::fail("The handle is out of range (" + std::to_string(handle.id) + " against " + std::to_string(_slots.size()) + ")- can't destroy the object assiociated to it.");

                    Slot& slot = _slots.at(handle.id);

                    if (slot.generation != handle.generation)
                        return ml::ret::fail("The slot generation and the handle generation don't match (handle generation : " + std::to_string(handle.generation) + " slot generation : " + std::to_string(slot.generation) + ") - can't destroy the object assiociated to it.");
                    if (!slot.object.has_value())
                        return ml::ret::fail("The slot object has no value - nothing to do");

                    slot.object.reset();
                    ++slot.generation;
                    if (slot.generation == 0)
                        ++slot.generation;

                    _free.push_back(handle.id);
                    return ml::ret::success();
                }
    };

    template<typename T>
        T* Handle<T>::get()
        {
            if (!_manager)
                return nullptr;

            return _manager->get(*this);
        }

    template<typename T>
        const T* Handle<T>::get() const
        {
            if (!_manager)
                return nullptr;

            return static_cast<const ObjectsManager*>(_manager)->get(*this);
        }
}
