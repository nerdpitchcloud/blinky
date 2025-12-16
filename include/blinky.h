#ifndef BLINKY_H
#define BLINKY_H

#include <string>

namespace blinky {

enum class Status {
    GREEN,
    RED,
    YELLOW
};

class Monitor {
public:
    Monitor();
    ~Monitor();
    
    Status getStatus() const;
    void setStatus(Status status);
    std::string getStatusString() const;
    void display() const;

private:
    Status current_status;
};

}

#endif
