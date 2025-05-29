#include "fft_utils.h"

void _fft(cplx buf[], cplx out[], int n, int step)
{
    if (step < n)
    {
        _fft(out, buf, n, step * 2);
        _fft(out + step, buf + step, n, step * 2);

        for (int i = 0; i < n; i += 2 * step)
        {
            cplx t = cexpf(-I * (M_PI * i) / n) * out[i + step];
            buf[i / 2] = out[i] + t;
            buf[(i + n) / 2] = out[i] - t;
        }
    }
}

void fft(cplx buf[], cplx out[], int n)
{
    for (int i = 0; i < n; i++)
    {
        out[i] = buf[i];
    }

    _fft(buf, out, n, 1);
}