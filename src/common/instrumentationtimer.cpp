#include "instrumentationtimer.h"
#include <iostream>

using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

InstrumentationTimer::InstrumentationTimer(
    const char *name)
    : m_Name(name),
      m_StartTimepoint(std::chrono::steady_clock::now()),
      m_Stopped(false)
{}

InstrumentationTimer::~InstrumentationTimer()
{
    if (!m_Stopped)
        Stop();
}

void InstrumentationTimer::Stop()
{
    auto endTimepoint = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

    std::cout << "Rendered " << m_Name << " in " << elapsedTime.count() / 1000.f << "ms" << std::endl;

    m_Stopped = true;
}
