#include "./SSEEventLoop.h"
#include "./network.h"

SSEEventLoop::~SSEEventLoop(){}

void SSEEventLoop::_init()
{
    auto onsse = [this]
    {
        lg("sse-event triggered !");
        json data;
        try
        {
            data = std::any_cast<json>(_events.data());
        }
        catch(const std::exception& e)
        {
            lg("error getting the json data in the SSEEventLoop : ");
            lg(e.what());
        }
        {
            std::lock_guard lk(_subscribers);
            lg("Writing the data " << data.dump(4) << " to " << _subscribers.data().size() << " subscribers.");
            auto& subs = _subscribers.data();
            for (int i = subs.size() - 1; i >= 0; i--)
            {
                bool alive = _server->sendAsSSE(subs[i], data.dump());
                if (!alive)
                    subs.removeByIndex(i);
            }
            lg("data sended.");
        }
    };
    _events.add("sse-event", onsse);
}

void SSEEventLoop::run()
{
    while(true)	
    {
        lg("Waiting for an event to occur...");
        auto type = _events.wait(std::vector<std::string>{ "sse-event", "stop"});
        if (type == "stop")
        {
            lg("stop event triggered.");
            lg("abort.");
            break;
        }
        else 
        {
            lg("Event " << type << " triggered.");
            lg("The event should be triggered automaticly.");
        }
        if (_stop)
            break;
    }
}

void SSEEventLoop::stop()
{
    _events.emit("stop");
    _stop = true;	
}

void SSEEventLoop::addSubscriber(std::shared_ptr<tcp::socket> s)
{
    lg("Adding a new _subscriber " << s.get());
    auto headers = network::sse_headers();
    _server->write(s, headers);
    lg("sse headers sent.");
    std::lock_guard lk(_subscribers);
    _subscribers.data().push_back(s);
    lg("subcriber added to the list.");
}

void SSEEventLoop::removeSubscriber(std::shared_ptr<tcp::socket> s)
{
    std::lock_guard lk(_subscribers);
    for (int i = 0; i < _subscribers.data().size(); i++)
    {
        auto& _s = _subscribers.data()[i];
        if (_s.get() == s.get())
        {
            _subscribers.data().removeByIndex(i);
            break;
        }
    }
}

void SSEEventLoop::emit(const json& data)
{
    _events.emit("sse-event", std::any(data));	
}
