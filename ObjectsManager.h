#pragma once
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>
#include "./Ret.h"

namespace ml
{
    template <typename T>
        struct Handle
        {
            unsigned int id = std::numeric_limits<unsigned int>::max();
            unsigned int generation = 0;

            bool valid() const
            {
                return id != std::numeric_limits<unsigned int>::max() && generation != 0;
            }

            explicit operator bool() const
            {
                return this->valid();
            }
        };

    template <typename T>
        struct Slot
        {
            std::unique_ptr<T> object;
            unsigned int generation = 1;
        };

    template <typename T>
        class ObjectsManager
        {
            private : 
                std::vector<Slot<T>> _slots; 
                std::vector<unsigned int> _free;

            public : 
                ObjectsManager()
                {
                    static_assert(
                            std::has_virtual_destructor_v<T> ,
                            "T must have a virtual destructor"
                            );
                }

                template <typename S, typename ... Args>
                    Handle<S> create(Args&& ... args)
                    {
                        static_assert(
                                std::is_base_of_v<T, S>,
                                "S must inherit from T"
                                );

                        unsigned int id;
                        if (_free.empty())
                        {
                            id = _slots.size();
                            _slots.emplace_back();
                        }
                        else 
                        {
                            id = _free.back();
                            _free.pop_back();
                        }

                        Slot<T>& slot = _slots.at(id);
                        slot.object = std::make_unique<S>(std::forward<Args>(args)...);
                        return {id, slot.generation};
                    } 

                template <typename S>
                    S* get(const Handle<S>& handle)
                    {
                        if (!handle.valid())
                            return nullptr;
                        if (handle.id >= _slots.size())
                            return nullptr;
                        Slot<T>& slot = _slots[handle.id];
                        if (slot.generation != handle.generation)
                            return nullptr;
                        if (!slot.object)
                            return nullptr;

                        return dynamic_cast<S*>(slot.object.get());
                    }

                template <typename S>
                    const S* get(const Handle<S>& handle) const
                    {
                        if (!handle.valid())
                            return nullptr;
                        if (handle.id >= _slots.size())
                            return nullptr;
                        const Slot<T>& slot = _slots[handle.id];
                        if (slot.generation != handle.generation)
                            return nullptr;
                        if (!slot.object)
                            return nullptr;

                        return dynamic_cast<const S*>(slot.object.get());
                    }

                template <typename S>
                    ml::Ret<> destroy(const Handle<S>& handle)
                    {
                        if (!handle.valid()) 
                            return ml::ret::fail("The handle is non-valid - can't destroy the object assiociated to it.");
                        if (handle.id >= _slots.size())
                            return ml::ret::fail("The handle is out of range (" + std::to_string(handle.id) + " against " + std::to_string(_slots.size()) + ")- can't destroy the object assiociated to it.");

                        Slot<T>& slot = _slots.at(handle.id);

                        if (slot.generation != handle.generation)
                            return ml::ret::fail("The slot generation and the handle generation don't match (handle generation : " + std::to_string(handle.generation) + " slot generation : " + std::to_string(slot.generation) + ") - can't destroy the object assiociated to it.");
                        if (!slot.object)
                            return ml::ret::fail("The slot object is nullptr - nothing to do");

                        slot.object.reset();
                        ++slot.generation;
                        if (slot.generation == 0)
                            ++slot.generation;

                        _free.push_back(handle.id);
                        return ml::ret::success();
                    }
        };
}
