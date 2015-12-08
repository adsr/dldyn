#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include "dyncall/dyncall/dyncall.h"

#define DLDYN_VERSION "0.1"

typedef void (*dcCallFn)(DCCallVM*, DCpointer);

typedef struct dldyn_raw_s {
    unsigned char* data;
    size_t size;
    struct dldyn_raw_s* prev;
} dldyn_raw_t;

dldyn_raw_t* dldyn_raw_head = NULL;

static void do_call(DCCallVM* vm, char* return_type, DCpointer func, int ptr_size);
static void free_raw();
static DCpointer make_raw(char* hex);
static void make_raw_string(dldyn_raw_t* raw, char* str);
static void make_raw_hex(dldyn_raw_t* raw, char* hex);

static void show_help() {
    printf(
        "dldyn version %s\n"
        "\n"
        "Usage: dldyn -m <module_path> -f <func_name> [options]\n"
        "\n"
        "    -h           Show this help\n"
        "    -m <path>    Path of module\n"
        "    -f <func>    Function name\n"
        "    -r <type>    Return type of function\n"
        "    -b <val>     Add bool param\n"
        "    -c <val>     Add char param\n"
        "    -s <val>     Add short param\n"
        "    -i <val>     Add int param\n"
        "    -l <val>     Add long param\n"
        "    -L <val>     Add long long param\n"
        "    -F <val>     Add float param\n"
        "    -d <val>     Add double param\n"
        "    -p <hex>     Add pointer param\n"
        "    -S <val>     Pointer size of return value\n"
        "\n"
        "    type:        bool, char, short, int, long, longlong,\n"
        "                 float, double, pointer, string, void\n"
        "\n"
        "    hex:         0x([A-Fa-f]{2})+\n"
        "                 null\n",
        DLDYN_VERSION
    );
}

int main(int argc, char** argv) {
    DCCallVM* vm = NULL;
    char* return_type = "string";
    char* module_path = NULL;
    char* func_name = NULL;
    void* dlh = NULL;
    void* dlfunc = NULL;
    int ptr_size = 0;
    int exit_code = EXIT_SUCCESS;

    vm = dcNewCallVM(4096);
    dcMode(vm, DC_CALL_C_DEFAULT);
    dcReset(vm);

    while (optind < argc) {
        switch (getopt(argc, argv, "hm:f:r:b:c:s:i:l:L:F:d:p:S:")) {
            case 'h':
                show_help();
                goto cleanup;
            case 'm':
                module_path = optarg;
                break;
            case 'f':
                func_name = optarg;
                break;
            case 'r':
                return_type = optarg;
                break;
            case 'b':
                dcArgBool(vm, (DCbool)atoi(optarg));
                break;
            case 'c':
                dcArgChar(vm, (DCchar)atoi(optarg));
                break;
            case 's':
                dcArgShort(vm, (DCshort)strtod(optarg, NULL));
                break;
            case 'i':
                dcArgInt(vm, (DCint)strtod(optarg, NULL));
                break;
            case 'l':
                dcArgLong(vm, (DClong)strtol(optarg, NULL, 10));
                break;
            case 'L':
                dcArgLongLong(vm, (DClonglong)strtoll(optarg, NULL, 10));
                break;
            case 'F':
                dcArgFloat(vm, (DCfloat)atof(optarg));
                break;
            case 'd':
                dcArgDouble(vm, (DCdouble)atof(optarg));
                break;
            case 'p':
                dcArgPointer(vm, make_raw(optarg));
                break;
            case 'S':
                ptr_size = (int)strtoul(optarg, NULL, 10);
        }
    }

    if (!module_path || !func_name) {
        fprintf(stderr, "Both -m (module path) and -f (function name) are required\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    if (!(dlh = dlopen(module_path, RTLD_LAZY))) {
        printf("dlopen error: %s\n", dlerror());
        exit_code = EXIT_FAILURE;
        goto cleanup;
    } else if (!(dlfunc = dlsym(dlh, func_name))) {
        printf("dlsym error: %s\n", dlerror());
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    do_call(vm, return_type, (DCpointer)dlfunc, ptr_size);

cleanup:
    if (dlh) dlclose(dlh);
    dcFree(vm);
    free_raw();
    return exit_code;
}

static void do_call(DCCallVM* vm, char* return_type, DCpointer func, int ptr_size) {
    DCpointer ptr;
    if (0 == strcmp(return_type, "bool")) {
        printf("%s\n", dcCallBool(vm, func) ? "true" : "false");
    } else if (0 == strcmp(return_type, "char")) {
        printf("%c\n", dcCallChar(vm, func));
    } else if (0 == strcmp(return_type, "short")) {
        printf("%hd\n", dcCallShort(vm, func));
    } else if (0 == strcmp(return_type, "int")) {
        printf("%d\n", dcCallInt(vm, func));
    } else if (0 == strcmp(return_type, "long")) {
        printf("%ld\n", dcCallLong(vm, func));
    } else if (0 == strcmp(return_type, "longlong")) {
        printf("%lld\n", dcCallLongLong(vm, func));
    } else if (0 == strcmp(return_type, "float")) {
        printf("%f\n", dcCallFloat(vm, func));
    } else if (0 == strcmp(return_type, "double")) {
        printf("%f\n", dcCallDouble(vm, func));
    } else if (0 == strcmp(return_type, "pointer") || 0 == strcmp(return_type, "string")) {
        ptr = dcCallPointer(vm, func);
        if ('s' == *return_type) {
            printf("%s\n", (char*)ptr);
        } else if (ptr_size <= 0) {
            printf("%p\n", (void*)ptr);
        } else {
            while (ptr_size--) putchar(*(char*)ptr++);
        }
    } else {
        dcCallVoid(vm, func);
    }
}

static void free_raw() {
    dldyn_raw_t* tmp;
    while (dldyn_raw_head) {
        tmp = dldyn_raw_head;
        free(tmp->data);
        dldyn_raw_head = tmp->prev;
        free(tmp);
    }
}

static DCpointer make_raw(char* data) {
    dldyn_raw_t* raw;

    if (0 == strcasecmp(data, "null") || 0 == strcasecmp(data, "0x") || strlen(data) < 1) {
        return (DCpointer)NULL;
    }

    raw = calloc(1, sizeof(dldyn_raw_t));
    raw->prev = dldyn_raw_head;
    dldyn_raw_head = raw;

    if (0 == strncasecmp(data, "0x", 2)) {
        data += 2;
        make_raw_hex(raw, data);
    } else {
        make_raw_string(raw, data);
    }

    return (DCpointer)(raw->data);
}

static void make_raw_string(dldyn_raw_t* raw, char* str) {
    raw->data = (unsigned char*)strdup(str);
    raw->size = strlen((char*)raw->data);
}


static void make_raw_hex(dldyn_raw_t* raw, char* hex) {
    size_t hexlen, hexi;
    char hexbyte[3] = {0, 0, 0};
    unsigned char* dataptr;
    hexlen = strlen(hex);
    raw->data = calloc(hexlen / 2, sizeof(unsigned char));
    dataptr = raw->data;
    for (hexi = 0; hexi < hexlen; hexi += 2) {
        hexbyte[0] = hex[hexi];
        hexbyte[1] = hexi + 1 < hexlen ? hex[hexi + 1] : '0';
        *dataptr = strtoul(hexbyte, NULL, 16);
        dataptr++;
        raw->size += 1;
    }
}
