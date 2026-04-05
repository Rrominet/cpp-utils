//This is a generated file, don't change it manually, it will be override when rebuild.

bool detached(){return _detached;}
const bool detached() const {return _detached;}
void setDetached(bool detached){_detached = detached;}

int exitCode(){return _exitCode;}
const int exitCode() const {return _exitCode;}


std::string stdout(){return _stdout;}
const std::string& stdout() const {return _stdout;}


std::string stderr(){return _stderr;}
const std::string& stderr() const {return _stderr;}


std::string processPath(){return _processPath;}
const std::string& processPath() const {return _processPath;}
void setProcessPath(const std::string& processPath){_processPath = processPath;}

ml::Vec<std::string>& processArgs(){return _processArgs;}
const ml::Vec<std::string>& processArgs() const {return _processArgs;}
void setProcessArgs(const ml::Vec<std::string>& processArgs){_processArgs = processArgs;}

