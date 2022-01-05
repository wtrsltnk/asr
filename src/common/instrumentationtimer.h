#ifndef INSTRUMENTATIONTIMER_H
#define INSTRUMENTATIONTIMER_H

#include <chrono>

class InstrumentationTimer
{
public:
    explicit InstrumentationTimer(
        const char *name);

    ~InstrumentationTimer();

    void Stop();

private:
    const char *m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped;
};

#endif // INSTRUMENTATIONTIMER_H
