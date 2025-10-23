#include "git.h"
#include "mlprocess.h"

namespace git
{
    std::string _dir;
    std::string _activeBranch;
    // you need to call this before using any git function !
    void setDir(const std::string& dir){_dir = dir;}

    void checkDir()
    {
        if (_dir.empty())
            throw std::runtime_error("git::setDir must be called before any git function");
    }

    ml::Vec<std::string> branches()
    {
        git::checkDir();
        auto res = process::exec("git branch", _dir);
        ml::Vec<std::string>branches = str::split(res, "\n");
        if (branches.last() == "")
            branches.pop();

        for (auto& b : branches)
        {
            if (b[0] == '*')
                _activeBranch = b.substr(2);
        }
        for (auto& b : branches)
            b = b.substr(2); // remove '* ' or '  '

        return branches;
    }

    std::string activeBranch()
    {
        git::branches(); // this is setting _activeBranch under the hood.
        return _activeBranch;
    }

    void checkout(const std::string& branch)
    {
        git::checkDir();
        process::exec("git checkout \"" + branch + "\"", _dir);
    }

    void createBranch(const std::string& branch)
    {
        git::checkDir();
        process::exec("git branch \"" + branch + "\"", _dir);
    }

    ml::Vec<Commit> commits()
    {
        git::checkDir();
        auto _t = process::exec("git log", _dir);
        return {};
    }

    void commit(const std::string& message)
    {
        git::checkDir();
        process::exec("git commit -m \"" + message + "\"", _dir);
    }

    void revert(const std::string& commit)
    {
        git::checkDir();
        process::exec("git revert " + commit, _dir);
    }

    void branches_async(const std::function<void (const ml::Vec<std::string>)>& onDoned)
    {
        auto f = [onDoned]{
            onDoned(git::branches());
        };
        std::thread(f).detach();
    }
    void activeBranch_async(const std::function<void (std::string)>& onDoned)
    {
        auto f = [onDoned]{
            onDoned(git::activeBranch());
        };
        std::thread(f).detach();
    }
    void checkout_async(const std::string& branch, const std::function<void ()>& onDoned)
    {
        auto f = [onDoned, branch]{
            git::checkout(branch);
            onDoned();
        };
        std::thread(f).detach();
    }
    void createBranch_async(const std::string& branch, const std::function<void ()>& onDoned)
    {
        auto f = [onDoned, branch]{
            git::createBranch(branch);
            onDoned();
        };
        std::thread(f).detach();
    }
    void commits_async(const std::function<void (ml::Vec<Commit>)>& onDoned)
    {
        auto f = [onDoned]{
            onDoned(git::commits());
        };
        std::thread(f).detach();
    }
    void commit_async(const std::string& message, const std::function<void ()>& onDoned)
    {
        auto f = [onDoned, message]{
            git::commit(message);
            onDoned();
        };
        std::thread(f).detach();
    }
    void revert_async(const std::string& commit, const std::function<void ()>& onDoned)
    {
        auto f = [onDoned, commit]{
            git::revert(commit);
            onDoned();
        };
        std::thread(f).detach();
    }
}
