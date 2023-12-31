#ifndef __PrnTs_h_
#define __PrnTs_h_

void PrnTs(const char* fmt, ...);

extern"C++"
template <typename TElement, int N>
char (*
    RtlpNumberOf(TElement (&rparam)[N])
    )[N];

#define ARRAYSIZE(arr)    ( sizeof(*RtlpNumberOf(arr)) )


#endif