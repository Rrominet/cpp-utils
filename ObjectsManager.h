#pragma once
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>
#include <any>
#include "./Ret.h"
#include <unordered_map>
#include <typeindex>
#include <functional>

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
                                 _pointer(static_cast<T*>(other._pointer)){}

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

                T* get() const;
                void log() const
                {
                    lg(" -- Handle --");
                    lg2("Id", id);
                    lg2("Generation", generation);
                    lg2("Valid", valid());
                    lg2("Pointer", _pointer);
                    lg2("Manager", _manager);
                    lg(" -- -- ");
                }

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
                    _pointer(pointer){}

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

    template <typename T>
        bool operator==(const Handle<T>& lhs, const Handle<T>& rhs)
        {
            return (lhs.id == rhs.id && lhs.generation == rhs.generation && lhs._manager == rhs._manager && lhs._pointer == rhs._pointer);
        }

    template <typename T>
        bool operator!=(const Handle<T>& lhs, const Handle<T>& rhs)
        {
            return !(lhs == rhs);
        }

    class ObjectsManager
    {
        private : 
            using CreateCallback = std::function<void(void*)>;

            std::unordered_map<
                std::type_index,
                std::vector<CreateCallback>
            > _createCallbacks;

            std::vector<Slot> _slots; 
            std::vector<unsigned int> _free;

            template<typename S>
                bool _checkHandle(const Handle<S> handle) const
                {
                    lg("ObjectsManager::_checkHandle");
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
            const std::vector<Slot>& slots() const { return _slots; }

            //add function you want to execute on specefic types just after creation
            //this let you have "initialize" function that can have a valid handle of the object 
            //BECAUSE the handle is invalid during object constructor.
            //
            template<typename T, typename Callback>
                void registerInitFunc(Callback&& callback)
                {
                    auto wrapper =
                        [callback = std::forward<Callback>(callback)](void* object) mutable
                        {
                            callback(static_cast<T*>(object));
                        };

                    _createCallbacks[typeid(T)].push_back(std::move(wrapper));
                }

            //P is a parent class from S that you would want to have in the handle (not the child)
            //Useful for retrieving it from the handle
            template <typename S, typename P=S, typename ... Args>
                Handle<S> create(Args&& ... args)
                {
                    lg("ObjectsManager::create");
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
                    std::shared_ptr<P> parent = std::move(object);
                    slot.object = std::move(parent);

                    auto _r = Handle<S>(
                            this,
                            id,
                            slot.generation,
                            pointer
                            );

                    auto it = _createCallbacks.find(typeid(P));
                    if (it != _createCallbacks.end())
                    {
                        for (auto& callback : it->second)
                            callback(pointer);
                    }

                    return _r ;
                } 

            template <typename S>
                Handle<S> handle(S* pointer) 
                {
                    lg("ObjectsManager::handle");
                    lg("Number of slots : " << _slots.size());
                    for (unsigned int i = 0; i < _slots.size(); ++i)
                    {
                        const Slot& slot = _slots[i];

                        if (slot.object.has_value())
                        {
                            lg("Slot " << i << " is a shared_ptr<S>");
                            if (slot.object.type() != typeid(std::shared_ptr<S>))
                            {
                                lg("Types don't matches.");
                                lg("Slot " << i << " is a " << slot.object.type().name());
                                lg("Expected " << typeid(std::shared_ptr<S>).name());
                                continue;
                            }
                            auto ptr = std::any_cast<std::shared_ptr<S>>(slot.object);
                            if (ptr.get() != pointer)
                            {
                                lg("Pointer don't matches.");
                                lg("Slot " << i << " points to " << ptr.get());
                                lg("Expected " << pointer);
                                continue;
                            }

                            return Handle<S>(
                                    this,
                                    i,
                                    slot.generation,
                                    ptr.get()
                                    );
                        }
                        else 
                        {
                            lg("Slot " << i << " has no value.");
                        }
                    }

                    return Handle<S>();
                }

            template<typename S>
                S* get(const Handle<S>& handle) const
                {
                    lg("ObjectsManager::get");
                    if (!_checkHandle(handle))
                    {
                        lg("Handle " << handle.id << " is not valid.");
                        return nullptr;
                    }

                    const Slot& slot = _slots[handle.id];

                    if (!_checkSlot(slot, handle.generation))
                    {
                        lg("Slot " << handle.id << " is not valid.");
                        return nullptr;
                    }

                    return handle._pointer;
                }

            template <typename S>
                ml::Ret<> destroy(const Handle<S>& handle)
                {
                    lg("ObjectsManager::destroy");
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
        T* Handle<T>::get()const
        {
            lg("Handle::get()");
            if (!_manager)
            {
                lg("Manager is null, this should definitly NOT happen");
                return nullptr;
            }

            return _manager->get(*this);
        }

    namespace managed
    {
        template<typename T> 
            std::vector<T*> fromVector(ObjectsManager* manager, const std::vector<ml::Handle<T>>& handles)
            {
                std::vector<T*> objects;
                for (const auto& handle : handles)
                {
                    auto object = handle.get();
                    if (object)
                        objects.push_back(object);
                }
                return objects;
            }
    }
}
