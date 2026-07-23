#include "./ComputeEngine.h"
#include "./Node.h"

namespace ml
{
    namespace nodes
    {
        void ComputeEngine::computeList(const ml::Vec<Node*>& list)
        {
            for (int64_t i = list.size() - 1; i >= 0; i--)
            {
                if (!_computed.contains(list[i]))
                {
                    list[i]->__execute__();
                    _computed.push_back(list[i]);
                }
            }
        }

        void ComputeEngine::compute(const ml::Vec<ml::Vec<Node*>>& execlist)
        {
            _computed.clear();
            for (const auto& l : execlist)
                this->computeList(l);
        }
    }
}
