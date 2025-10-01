#pragma once
#include "vec.h"
#include "str.h"
#include <functional>

namespace git
{
    struct Commit
    {
        std::string hash;
        std::string author;
        std::string date;
        std::string message;
    };

    // you need to call this before using any git function !
    void setDir(const std::string& dir);

    ml::Vec<std::string> branches(); 
    std::string activeBranch();
    void checkout(const std::string& branch);
    void createBranch(const std::string& branch);
    ml::Vec<Commit> commits();
    void commit(const std::string& message);
    void revert(const std::string& commit);

    void branches_async(const std::function<void (const ml::Vec<std::string>)>& onDoned); 
    void activeBranch_async(const std::function<void (std::string)>& onDoned);
    void checkout_async(const std::string& branch, const std::function<void ()>& onDoned);
    void createBranch_async(const std::string& branch, const std::function<void ()>& onDoned);
    void commits_async(const std::function<void (ml::Vec<Commit>)>& onDoned);
    void commit_async(const std::string& message, const std::function<void ()>& onDoned);
    void revert_async(const std::string& commit, const std::function<void ()>& onDoned);
}
