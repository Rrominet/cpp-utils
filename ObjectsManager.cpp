#include "./ObjectsManager.h"

namespace ml
{
    bool ObjectsManager::_checkSlot(const Slot& slot, unsigned int generation) const
    {
        lg("ObjectsManager::_checkSlot");
        if (slot.generation != generation)
        {
            lg("Slot generation and the handle generation don't match (handle generation : " << generation << " slot generation : " << slot.generation << ")");
            return false;
        }
        if (!slot.object.has_value())
        {
            lg("Slot has no value.");
            return false;
        }
        return true;
    }

    ml::Ret<> ObjectsManager::destroy(unsigned int id, unsigned int generation, bool checkBounds)
    {
        lg("ObjectsManager::destroy");
        if (checkBounds)
        {
            if (id >= _slots.size())
                return ml::ret::fail("The id is out of range (" + std::to_string(id) + " against " + std::to_string(_slots.size()) + ")- can't destroy the object assiociated to it.");
        }

        Slot& slot = _slots[id];

        if (generation > 0)
        {
            if (slot.generation != generation)
                return ml::ret::fail("The slot generation and the handle generation don't match (handle generation : " + std::to_string(generation) + " slot generation : " + std::to_string(slot.generation) + ") - can't destroy the object assiociated to it.");
        }

        if (!slot.object.has_value())
            return ml::ret::fail("The slot object has no value - nothing to do");

        slot.object.reset();
        ++slot.generation;
        if (slot.generation == 0)
            ++slot.generation;

        _free.push_back(id);
        return ml::ret::success();
    }

    ml::Ret<> ObjectsManager::clear()
    {
        lg("ObjectsManager::clear");        
        for (unsigned int i = 0; i < _slots.size(); ++i)
            this->destroy(i, 0, false);
        return ml::ret::success();
    }
}
