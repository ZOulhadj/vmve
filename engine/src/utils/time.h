#ifndef TIME_H
#define TIME_H

namespace engine
{
    class time
    {
    public:
        time() = default;

        // Calculates the time difference from the previous frame. This function
        // must only be called once at the start of a frame.
        void calculate_delta_time();

        float get_delta_time() const;
    private:
        float m_delta_time;
    };
}


#endif