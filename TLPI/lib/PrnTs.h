#ifndef __PrnTs_h_
#define __PrnTs_h_

#ifdef __cplusplus
extern"C"{
#endif


void PrnTs(const char* fmt, ...);

const char* strsigname(int signo);

void print_my_PXIDs();

	
extern"C++"
template <typename TElement, int N>
char (*
    RtlpNumberOf(TElement (&rparam)[N])
    )[N];

#define ARRAYSIZE(arr)    ( sizeof(*RtlpNumberOf(arr)) )



#ifdef __cplusplus
} // extern"C" {
#endif

#endif