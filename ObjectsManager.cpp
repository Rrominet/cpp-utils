#include "./ObjectsManager.h"

namespace ml
{
    bool ObjectsManager::_checkSlot(const Slot& slot, unsigned int generation) const
    {
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
}
