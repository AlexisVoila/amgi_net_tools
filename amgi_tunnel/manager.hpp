#ifndef MANAGER_H
#define MANAGER_H

#include <set>
#include <memory>

class session_base 
{
public:
    virtual ~session_base() = default;
    virtual void stop() {}
};

using session_base_ptr = std::shared_ptr<session_base>;

class session_manager 
{
public:
    void join(session_base_ptr ses) 
    {
        sessions_.insert(std::move(ses));
    }

    void leave(const session_base_ptr& ses)
    {
        ses->stop();
        sessions_.erase(ses);
    }

    std::size_t ses_count() { return sessions_.size(); }
private:
    std::set<session_base_ptr> sessions_;
};



#endif //MANAGER_H