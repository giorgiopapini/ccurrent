#ifndef CCURRENT_H
#define CCURRENT_H

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    #define CC_POSIX
    #include <pthread.h>
    #include <stdint.h>

    #define CC_TH_FUNC_RET      void *
    #define CC_TH_RETURN(val)   return (void *)(intptr_t)val

    typedef pthread_t cc_th;
    typedef pthread_t cc_th_id;
    typedef pthread_attr_t cc_th_attr;
    typedef pthread_key_t cc_tls_key;
    typedef void *(*cc_th_func)(void *);

#elif defined(_WIN32)
    #define CC_WINDOWS
    #include <windows.h>

    #define WINDOWS_DEFAULT_GUARD_SIZE  4096

    #define CC_TH_FUNC_RET      DWORD WINAPI
    #define CC_TH_RETURN(val)   return (DWORD)(ULONG_PTR)val

    typedef HANDLE cc_th;
    typedef DWORD cc_th_id;
    typedef DWORD cc_tls_key;
    typedef DWORD (WINAPI *cc_th_func)(LPVOID);

    typedef struct {
        int detach_state;
        size_t stack_size;
    } cc_th_attr;

#endif

/* attribute possible values */
#define CC_TH_JOINABLE      0
#define CC_TH_DETACHED      1

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CC_POSIX)
static inline int cc_th_attr_destroy(cc_th_attr *p_attr) { return pthread_attr_destroy(p_attr); }
/* loads the detach state inside p_res integer pointer */
static inline int cc_th_attr_getdetachstate(cc_th_attr *p_attr, int *p_detachstate) {
    return pthread_attr_getdetachstate(p_attr, p_detachstate);
}
static inline int cc_th_attr_getguardsize(cc_th_attr *p_attr, size_t *p_guardsize) {
    return pthread_attr_getguardsize(p_attr, p_guardsize);
}
static inline int cc_th_attr_getstacksize(cc_th_attr *p_attr, size_t *p_stacksize) {
    return pthread_attr_getstacksize(p_attr, p_stacksize);
}
static inline int cc_th_attr_init(cc_th_attr *p_attr) { return pthread_attr_init(p_attr); }
static inline int cc_th_attr_setdetachstate(cc_th_attr *p_attr, int detachstate) {
    return pthread_attr_setdetachstate(p_attr, detachstate);
}
static inline int cc_th_attr_setguardsize(cc_th_attr *p_attr, size_t guardsize) {
    return pthread_attr_setguardsize(p_attr, guardsize);
}
static inline int cc_th_attr_setstacksize(cc_th_attr *p_attr, size_t stacksize) {
    return pthread_attr_setstacksize(p_attr, stacksize);
}
static inline int cc_th_create(cc_th *p_th, cc_th_attr *p_attr, void *(*th_func)(void *), void *arg) {
    return pthread_create(p_th, p_attr, th_func, arg);
}
static inline int cc_th_detach(cc_th *p_th) {
    if (!p_th) return -1;
    return pthread_detach(*p_th);
}
static inline int cc_th_equal_id(cc_th_id id1, cc_th_id id2) { return pthread_equal(id1, id2); }
static inline void cc_th_exit(void *retval) { pthread_exit(retval); }
static inline int cc_th_join(cc_th th, void **retval) { return pthread_join(th, retval); }
static inline void *cc_tls_get(cc_tls_key key) { return pthread_getspecific(key); }
static inline int cc_tls_key_create(cc_tls_key *p_key) {
    if (!p_key) return -1;
    return pthread_key_create(p_key, NULL);
}
static inline int cc_tls_key_delete(cc_tls_key key) { return pthread_key_delete(key); }
static inline cc_th_id cc_th_self(void) { return pthread_self(); }
static inline int cc_tls_set(cc_tls_key key, void *value) { return pthread_setspecific(key, value); }

#elif defined(CC_WINDOWS)
static inline int cc_th_attr_destroy(cc_th_attr *p_attr) { return 0; }  /* nop */
static inline int cc_th_attr_getdetachstate(cc_th_attr *p_attr, int *p_detachstate) {
    if (!p_detachstate || !p_attr) return -1;
    *(p_detachstate) = p_attr->detach_state;
    return 0;
}
static inline int cc_th_attr_getguardsize(cc_th_attr *p_attr, size_t *p_guardsize) {
    if (!p_guardsize || !p_attr) return -1;
    *(p_guardsize) = WINDOWS_DEFAULT_GUARD_SIZE;
    return 0;
}
static inline int cc_th_attr_getstacksize(cc_th_attr *p_attr, size_t *p_stacksize) {
    if (!p_stacksize || !p_attr) return -1;
    *(p_stacksize) = p_attr->stack_size;
    return 0;
}
static inline int cc_th_attr_init(cc_th_attr *p_attr) {
    if (!p_attr) return -1;
    p_attr->detach_state = CC_TH_JOINABLE;
    p_attr->stack_size = 0;
    return 0;
}
static inline int cc_th_attr_setdetachstate(cc_th_attr *p_attr, int detachstate) {
    if (!p_attr) return -1;
    p_attr->detach_state = detachstate;
    return 0;
}
static inline int cc_th_attr_setguardsize(cc_th_attr *p_attr, size_t guardsize) { return 0; }  /* nop */
static inline int cc_th_attr_setstacksize(cc_th_attr *p_attr, size_t stacksize) {
    if (!p_attr) return -1;
    p_attr->stack_size = stacksize;
    return 0;
}
static inline int cc_th_create(cc_th *p_th, cc_th_attr *p_attr, void *(*th_func)(void *), void *arg) {
    SIZE_T actual_stacksize = 0;
    if (p_attr && p_attr->stack_size > 0)
        actual_stacksize = (SIZE_T)p_attr->stack_size;

    *p_th = CreateThread(NULL,
        actual_stacksize,
        (LPTHREAD_START_ROUTINE)th_func,
        (LPVOID)arg,
        0,
        NULL
    );

    if (!*p_th) return -1;
    if (p_attr && CC_TH_DETACHED == p_attr->detach_state) {
        if (!CloseHandle(*p_th)) return -2;
        *p_th = NULL;
    }
    return 0;
}
static inline int cc_th_detach(cc_th *p_th) {
    if (!p_th) return -1;
    if (!*p_th) return 0;
    if (!CloseHandle(*p_th)) return -2;
    *p_th = NULL;
    return 0;
}
static inline int cc_th_equal_id(cc_th_id id1, cc_th_id id2) { return (id1 == id2); }
/* NOTE: retval should be a small integer status code. Code doesn't work for words larger than DWORD_MAX */
static inline void cc_th_exit(void *retval) { ExitThread((DWORD)(ULONG_PTR)retval); }
static inline int cc_th_join(cc_th th, void **retval) {
    DWORD exit_code;
    DWORD wait_result = WaitForSingleObject(th, INFINITE);

    if (WAIT_OBJECT_0 == wait_result) {
        if (GetExitCodeThread(th, &exit_code)) {
            if (retval != NULL) *retval = (void*)(ULONG_PTR)exit_code;
            if (!CloseHandle(th)) return -2;
            return 0;
        }
        else return -1;
    }
    else if (WAIT_ABANDONED == wait_result) return -3;
    else if (WAIT_TIMEOUT == wait_result) return -4;
    else return -5;
}
static inline void *cc_tls_get(cc_tls_key key) { return TlsGetValue(key); }
/* NOTE: Use "cc_tls_cleanup" to deallocate memory per thread */
static inline int cc_tls_key_create(cc_tls_key *p_key) {
    if (!p_key) return -1;
    *p_key = TlsAlloc();
    if (*p_key == TLS_OUT_OF_INDEXES) return -1;
    return 0;
}
static inline int cc_tls_key_delete(cc_tls_key key) {
    if (!TlsFree(key)) return -1;
    return 0;
}
static inline cc_th_id cc_th_self(void) { return GetCurrentThreadId(); }
static inline int cc_tls_set(cc_tls_key key, void *value) {
    if (!TlsSetValue(key, value)) return -1;
    return 0;
}

#endif

#define cc_tls_cleanup(key, free_func) \
    do { \
        void *val = cc_tls_get(key); \
        if (val) { \
            free_func(val); \
            cc_tls_set(key, NULL); \
        } \
    } while(0)


#ifdef __cplusplus
}
#endif

#endif