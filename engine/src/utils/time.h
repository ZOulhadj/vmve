#ifndef TIME_H
#define TIME_H

namespace engine
{
    class time
    {
    public:
        time() = default;

        // Calculates the time difference from the previous frame. This allows
        // for frame dependent systems such as movement and translation
        // to run at the same speed no matter how fast the CPU runs.
        void calculate_delta_time();

        float get_delta_time() const;
    private:
        float m_delta_time;
    };
}


#endif