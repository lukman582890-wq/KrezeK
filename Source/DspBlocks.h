#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Pitch shifter klasik: dua tap pada delay line yang di-crossfade.
// ratio > 1 menaikkan pitch (2 = +1 oktaf, 4 = +2 oktaf).
class DelayPitchShifter
{
public:
    void prepare (double sampleRate)
    {
        window = 0.045 * sampleRate; // ~45 ms
        int size = 1;
        while (size < (int) window + 8)
            size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        phase = 0.0;
    }

    float process (float in, float ratio)
    {
        buffer[(size_t) writePos] = in;

        phase += (1.0 - (double) ratio) / window;
        phase -= std::floor (phase);

        double phase2 = phase + 0.5;
        phase2 -= std::floor (phase2);

        const float w1 = 0.5f - 0.5f * std::cos (6.28318530718f * (float) phase);
        const float w2 = 1.0f - w1;

        const float out = w1 * readAt (phase * window) + w2 * readAt (phase2 * window);
        writePos = (writePos + 1) & mask;
        return out;
    }

private:
    float readAt (double delay) const
    {
        const int d = (int) delay;
        const float frac = (float) (delay - (double) d);
        const int i0 = (writePos - d) & mask;
        const int i1 = (i0 - 1) & mask;
        return buffer[(size_t) i0] * (1.0f - frac) + buffer[(size_t) i1] * frac;
    }

    std::vector<float> buffer;
    int mask = 0, writePos = 0;
    double window = 2048.0, phase = 0.0;
};

// All-pass Schroeder dengan feedback (dipakai oleh kontrol SPEED).
class AllPass
{
public:
    void prepare (int lengthSamples)
    {
        len = std::max (1, lengthSamples);
        buf.assign ((size_t) len, 0.0f);
        pos = 0;
    }

    float process (float x, float g)
    {
        const float d = buf[(size_t) pos];
        const float v = x + g * d;
        const float y = d - g * v;
        buf[(size_t) pos] = v;
        pos = (pos + 1) % len;
        return y;
    }

private:
    std::vector<float> buf;
    int len = 1, pos = 0;
};
