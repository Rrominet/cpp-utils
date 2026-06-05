//This is a generated file, don't change it manually, it will be override when rebuild.

std::atomic<State>& state(){return _state;}
const std::atomic<State>& state() const {return _state;}


std::string path(){return _path;}
const std::string& path() const {return _path;}
void setPath(const std::string& path){_path = path;}

std::string executorPath(){return _executorPath;}
const std::string& executorPath() const {return _executorPath;}
void setExecutorPath(const std::string& executorPath){_executorPath = executorPath;}

std::unordered_map<std::string,ml::Vec<Plugin>>& plugins(){return _plugins;}
const std::unordered_map<std::string,ml::Vec<Plugin>>& plugins() const {return _plugins;}


