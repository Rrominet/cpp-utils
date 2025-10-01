#pragma once
    void insert(const basic_json& val, int idx=0)
    {
        try
        {
            insert(begin() + idx, val);
        }
        catch(const json::type_error& e)
        {
            push_back(val);
        }
        catch(const std::exception& e)
        {
            throw;
        }
    }

    void insert(basic_json&& val, int idx=0){insert(val, idx);}
