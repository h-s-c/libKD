#include <KD/kd.h>
KD_API KDint KD_APIENTRY kdLogMessagefKHR(const KDchar *format, ...);

#if defined(__clang__)
#if __has_warning("-Wliteral-conversion")
#pragma clang diagnostic ignored "-Wliteral-conversion"
#endif
#endif

#define TEST_EQ(a, b) do {\
    if(!((a) == (b))) {\
        kdLogMessagefKHR("%s:%d (%d != %d)\n", __FILE__, __LINE__, (a), (b));\
        kdExit(-1);\
}} while (0)

#define TEST_APPROXF(a, b) do {\
    if(!(((b) ? kdFabsf(((b) - (a)) / (b)) : kdFabsf(a) ) < (KD_FLT_EPSILON * 4))) {\
        kdLogMessagefKHR("%s:%d (%.9g != %.9g)\n", __FILE__, __LINE__, (a), (b));\
        kdExit(-1);\
}} while (0)

#define TEST_APPROX(a, b) do {\
    if(!(((b) ? kdFabsKHR(((b) - (a)) / (b)) : kdFabsKHR(a) ) < (KD_DBL_EPSILON_KHR * 4))) {\
        kdLogMessagefKHR("%s:%d (%.17g != %.17g)\n", __FILE__, __LINE__, (a), (b));\
        kdExit(-1);\
}} while (0)

#define TEST_EXPR(expr) do {\
    if(!(expr)) {\
        kdLogMessagefKHR("%s:%d (%s)\n", __FILE__, __LINE__, (#expr));\
        kdExit(-1);\
}} while (0)

#define TEST_STREQ(a, b) do {\
    if(!(kdStrcmp(a, b) == 0)) {\
        kdLogMessagefKHR("%s:%d (%s != %s)\n", __FILE__, __LINE__, (a), (b));\
        kdExit(-1);\
}} while (0)

#define TEST_FAIL() do {\
    kdLogMessagefKHR("%s:%d\n", __FILE__, __LINE__);\
    kdExit(-1);\
} while (0)

#undef kdLogMessagefKHR