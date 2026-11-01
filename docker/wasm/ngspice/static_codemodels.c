/* Fritzing web app (docker/wasm): XSPICE code models linked into the app.
 *
 * ngspice loads code model libraries (analog.cm, ...) with dlopen(), which a static
 * WebAssembly build does not have. build-codemodels.sh compiles the analog code models with
 * the symbols of their dlmain.c prefixed with analogcm_, and the build makes dev.c use these
 * functions instead of dlopen(), dlsym() and dlerror(), so "codemodel analog.cm" works as
 * with the shared library.
 */

#include <string.h>

void *analogcm_CMdevs(void);
void *analogcm_CMdevNum(void);
void *analogcm_CMudns(void);
void *analogcm_CMudnNum(void);
void *analogcm_CMgetCoreItfPtr(void);

static int analog_handle;
static char error_message[] = "only analog.cm is available in this build";

void *static_cm_dlopen(const char *name, int mode)
{
    const char *base = strrchr(name, '/');
    (void) mode;
    base = base ? base + 1 : name;
    return strcmp(base, "analog.cm") == 0 ? &analog_handle : NULL;
}

void *static_cm_dlsym(void *library, const char *name)
{
    static const struct {
        const char *name;
        void *(*function)(void);
    } symbols[] = {
        { "CMdevs", analogcm_CMdevs },
        { "CMdevNum", analogcm_CMdevNum },
        { "CMudns", analogcm_CMudns },
        { "CMudnNum", analogcm_CMudnNum },
        { "CMgetCoreItfPtr", analogcm_CMgetCoreItfPtr },
    };
    size_t i;
    if (library != &analog_handle) return NULL;
    for (i = 0; i < sizeof symbols / sizeof symbols[0]; i++) {
        if (strcmp(symbols[i].name, name) == 0) return (void *) symbols[i].function;
    }
    return NULL;
}

char *static_cm_dlerror(void)
{
    return error_message;
}
