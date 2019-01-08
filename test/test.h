#include <KD/kd.h>
KD_API KDint KD_APIENTRY kdLogMessagefKHR(const KDchar *format, ...);

#define TEST_EQ(a, b) do {\
    if(!((a) == (b))) {\
        kdLogMessagefKHR("%s:%d (%d != %d)\n", __FILE__, __LINE__, (a), (b));\
        kdExit(-1);\
}} while (0)

#define TEST_EXPR(expr) do {\
    if(!(expr)) {\
        kdLogMessagefKHR("%s:%d (%d)\n", __FILE__, __LINE__, (#expr));\
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