#include "lib/unity.h"
#include "../src/ccurrent.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef CC_POSIX
#include <time.h>
#endif

// Define a test-specific TLS key
static cc_tls_key g_test_tls_key;

// --- Helper Functions for Tests ---

// Thread function for basic creation and joining
CC_TH_FUNC_RET thread_func_basic(void *arg) {
    int *val = (int *)arg;
    int ret_val = *val * 2;
    CC_TH_RETURN(ret_val);
}

// Thread function for detached threads
CC_TH_FUNC_RET thread_func_detached(void *arg) {
    (void)arg; // Unused
    // Simulate some work
    // In a real scenario, you might have a sleep here to ensure the main thread can proceed
    // before this thread potentially exits. For testing, a quick return is fine.
    CC_TH_RETURN(0);
}

// Thread function for TLS
CC_TH_FUNC_RET thread_func_tls(void *arg) {
    int *data = (int *)malloc(sizeof(int));
    TEST_ASSERT_NOT_NULL(data);
    *data = (int)(intptr_t)arg; // Cast arg to int, then to int* to store
    cc_tls_set(g_test_tls_key, data);

    // Verify TLS data
    int *retrieved_data = (int *)cc_tls_get(g_test_tls_key);
    TEST_ASSERT_EQUAL_INT(*data, *retrieved_data);

    cc_tls_cleanup(g_test_tls_key, free);
    
    CC_TH_RETURN(0);
}

// TLS cleanup function
void tls_free_func(void *data) {
    free(data);
}


// --- Test Cases ---

void setUp(void) {
    // This is run before each test
}

void tearDown(void) {
    // This is run after each test
}

// Test cc_th_attr_init and cc_th_attr_destroy
void test_cc_th_attr_init_and_destroy(void) {
    cc_th_attr attr;
    int result = cc_th_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, result);

    result = cc_th_attr_destroy(&attr);
    TEST_ASSERT_EQUAL_INT(0, result);
}

// Test cc_th_attr_setdetachstate and cc_th_attr_getdetachstate
void test_cc_th_attr_detachstate(void) {
    cc_th_attr attr;
    int result = cc_th_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, result);

    int detachstate;
    result = cc_th_attr_setdetachstate(&attr, CC_TH_DETACHED);
    TEST_ASSERT_EQUAL_INT(0, result);
    result = cc_th_attr_getdetachstate(&attr, &detachstate);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(CC_TH_DETACHED, detachstate);

    result = cc_th_attr_setdetachstate(&attr, CC_TH_JOINABLE);
    TEST_ASSERT_EQUAL_INT(0, result);
    result = cc_th_attr_getdetachstate(&attr, &detachstate);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(CC_TH_JOINABLE, detachstate);

    cc_th_attr_destroy(&attr);
}

// Test cc_th_attr_setstacksize and cc_th_attr_getstacksize
void test_cc_th_attr_stacksize(void) {
    cc_th_attr attr;
    int result = cc_th_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, result);

    size_t stack_size = 65536; // Example stack size
    result = cc_th_attr_setstacksize(&attr, stack_size);
    TEST_ASSERT_EQUAL_INT(0, result);

    size_t retrieved_stack_size;
    result = cc_th_attr_getstacksize(&attr, &retrieved_stack_size);
    TEST_ASSERT_EQUAL_INT(0, result);

#ifdef CC_POSIX
    TEST_ASSERT_EQUAL_UINT(stack_size, retrieved_stack_size);
#elif defined(CC_WINDOWS)
    // Windows might return 0 if it's the default, or the requested size if set explicitly.
    // We'll just check if it's not some arbitrary failure value.
    // For this test, we set it, so we expect it to be reflected.
    TEST_ASSERT_EQUAL_UINT(stack_size, retrieved_stack_size);
#endif

    cc_th_attr_destroy(&attr);
}

// Test cc_th_attr_setguardsize and cc_th_attr_getguardsize (POSIX specific, NOP on Windows)
void test_cc_th_attr_guardsize(void) {
    cc_th_attr attr;
    int result = cc_th_attr_init(&attr);
    TEST_ASSERT_EQUAL_INT(0, result);

    size_t guard_size = 4096; // Example guard size
    result = cc_th_attr_setguardsize(&attr, guard_size);
    TEST_ASSERT_EQUAL_INT(0, result);

    size_t retrieved_guard_size;
    result = cc_th_attr_getguardsize(&attr, &retrieved_guard_size);
    TEST_ASSERT_EQUAL_INT(0, result);

#ifdef CC_POSIX
    // POSIX should reflect the set guard size or a rounded-up value
    TEST_ASSERT_TRUE(retrieved_guard_size >= guard_size);
#elif defined(CC_WINDOWS)
    // On Windows, guard size is a NOP, so it should return the default.
    TEST_ASSERT_EQUAL_UINT(WINDOWS_DEFAULT_GUARD_SIZE, retrieved_guard_size);
#endif

    cc_th_attr_destroy(&attr);
}

// Test cc_th_create and cc_th_join
void test_cc_th_create_and_join(void) {
    cc_th th;
    int arg = 10;
    void *retval;

    int result = cc_th_create(&th, NULL, thread_func_basic, &arg);
    TEST_ASSERT_EQUAL_INT(0, result);

    result = cc_th_join(th, &retval);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(arg * 2, (int)(intptr_t)retval); // Cast back to int
}

// Test cc_th_create with attributes and join
void test_cc_th_create_with_attr_and_join(void) {
    cc_th th;
    cc_th_attr attr;
    int arg = 20;
    void *retval;

    cc_th_attr_init(&attr);
    cc_th_attr_setdetachstate(&attr, CC_TH_JOINABLE);
    cc_th_attr_setstacksize(&attr, 128 * 1024); // Set a custom stack size

    int result = cc_th_create(&th, &attr, thread_func_basic, &arg);
    TEST_ASSERT_EQUAL_INT(0, result);

    result = cc_th_join(th, &retval);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(arg * 2, (int)(intptr_t)retval);

    cc_th_attr_destroy(&attr);
}

// Test cc_th_detach
void test_cc_th_detach(void) {
    cc_th th;
    cc_th_attr attr;
    int arg = 30;

    cc_th_attr_init(&attr);
    cc_th_attr_setdetachstate(&attr, CC_TH_DETACHED);

    int result = cc_th_create(&th, &attr, thread_func_detached, &arg);
    TEST_ASSERT_EQUAL_INT(0, result);

    // For detached threads, cc_th_detach on the handle itself might not be needed
    // if the thread was created as detached. However, if created joinable and then
    // detached, this tests the function.
    // On Windows, a detached thread implicitly closes its handle on creation.
    // On POSIX, pthread_detach marks it as detached.
#ifdef CC_POSIX
    // If we create a joinable thread and then detach it manually
    cc_th th_joinable;
    cc_th_attr attr_joinable;
    cc_th_attr_init(&attr_joinable);
    cc_th_attr_setdetachstate(&attr_joinable, CC_TH_JOINABLE);
    result = cc_th_create(&th_joinable, &attr_joinable, thread_func_detached, &arg);
    TEST_ASSERT_EQUAL_INT(0, result);
    result = cc_th_detach(&th_joinable);
    TEST_ASSERT_EQUAL_INT(0, result);
    // Cannot join a detached thread, so no join test here.
    cc_th_attr_destroy(&attr_joinable);
#endif

    cc_th_attr_destroy(&attr);

    // Short sleep to allow detached thread to run and exit before test ends
    // This is a common heuristic for detached threads in tests, though not foolproof.
    // A better approach would be signaling, but for a basic detach test, it's often omitted.
#ifdef CC_POSIX
    nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);
#elif defined(CC_WINDOWS)
    Sleep(1); // 1 millisecond
#endif
}

// Test cc_th_equal_id
void test_cc_th_equal_id(void) {
    cc_th_id id1 = cc_th_self();
    cc_th_id id2 = cc_th_self();
    TEST_ASSERT_TRUE(cc_th_equal_id(id1, id2));

    // Cannot easily create a different thread's ID for comparison without
    // creating a new thread, which is done in other tests implicitly.
    // This test primarily ensures self-equality.
}

// Test cc_th_self
void test_cc_th_self(void) {
    cc_th_id current_thread_id = cc_th_self();
    // Simply checks that the function doesn't return a "null" or invalid ID.
    // Specific value depends on OS, so just non-zero/validity is enough.
#ifdef CC_POSIX
    TEST_ASSERT_TRUE(current_thread_id != 0); // pthread_t is opaque, but 0 is usually invalid
#elif defined(CC_WINDOWS)
    TEST_ASSERT_TRUE(current_thread_id != 0);
#endif
}

// Test cc_th_exit (hard to test directly without ending the test itself, usually covered by join)
// A conceptual test:
CC_TH_FUNC_RET thread_func_exit(void *arg) {
    int ret_val = (int)(intptr_t)arg;
    cc_th_exit((void *)(intptr_t)ret_val);
    // This line should not be reached
    TEST_FAIL_MESSAGE("cc_th_exit did not exit thread");
    CC_TH_RETURN(0); // Should not happen
}

void test_cc_th_exit(void) {
    cc_th th;
    int exit_code = 99;
    void *retval;

    int result = cc_th_create(&th, NULL, thread_func_exit, (void *)(intptr_t)exit_code);
    TEST_ASSERT_EQUAL_INT(0, result);

    result = cc_th_join(th, &retval);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(exit_code, (int)(intptr_t)retval);
}


// Test cc_tls_key_create and cc_tls_key_delete
void test_cc_tls_key_create_and_delete(void) {
    cc_tls_key key;
    int result = cc_tls_key_create(&key);
    TEST_ASSERT_EQUAL_INT(0, result);

    result = cc_tls_key_delete(key);
    TEST_ASSERT_EQUAL_INT(0, result);
}

// Test cc_tls_set and cc_tls_get within the same thread
void test_cc_tls_set_and_get_same_thread(void) {
    cc_tls_key key;
    cc_tls_key_create(&key);

    int data = 123;
    int result = cc_tls_set(key, (void *)(intptr_t)data);
    TEST_ASSERT_EQUAL_INT(0, result);

    void *retrieved_data = cc_tls_get(key);
    TEST_ASSERT_EQUAL_INT(data, (int)(intptr_t)retrieved_data);

    cc_tls_key_delete(key);
}

// Test cc_tls_set and cc_tls_get across multiple threads
void test_cc_tls_set_and_get_multiple_threads(void) {
    cc_tls_key_create(&g_test_tls_key); // Create a global key for threads to use

    cc_th th1, th2;
    int arg1 = 100;
    int arg2 = 200;

    int result = cc_th_create(&th1, NULL, thread_func_tls, (void *)(intptr_t)arg1);
    TEST_ASSERT_EQUAL_INT(0, result);
    result = cc_th_create(&th2, NULL, thread_func_tls, (void *)(intptr_t)arg2);
    TEST_ASSERT_EQUAL_INT(0, result);

    cc_th_join(th1, NULL);
    cc_th_join(th2, NULL);

    // Verify TLS is unique per thread by checking that current thread's TLS is NULL
    // if not set. If set, it's independent.
    TEST_ASSERT_NULL(cc_tls_get(g_test_tls_key));

    // Cleanup TLS data in each thread (done in thread_func_tls implicitly by return,
    // but demonstrating if needed for complex cleanup)
    // For this test, the thread function itself allocates and sets, so we rely on that.

    cc_tls_key_delete(g_test_tls_key);
}

// Test cc_tls_cleanup macro
void test_cc_tls_cleanup(void) {
    cc_tls_key key;
    int *data = (int *)malloc(sizeof(int));
    cc_tls_key_create(&key);

    
    TEST_ASSERT_NOT_NULL(data);
    *data = 55;
    cc_tls_set(key, data);

    // Ensure data is set
    TEST_ASSERT_EQUAL_PTR(data, cc_tls_get(key));

    // Use cleanup macro
    cc_tls_cleanup(key, free);

    // Ensure data is freed and TLS value is NULL
    TEST_ASSERT_NULL(cc_tls_get(key));

    cc_tls_key_delete(key);
}


// --- Main Test Runner ---
int main(void) {
    UNITY_BEGIN();

    // Attribute tests
    RUN_TEST(test_cc_th_attr_init_and_destroy);
    RUN_TEST(test_cc_th_attr_detachstate);
    RUN_TEST(test_cc_th_attr_stacksize);
    RUN_TEST(test_cc_th_attr_guardsize);

    // Thread creation and lifecycle tests
    RUN_TEST(test_cc_th_create_and_join);
    RUN_TEST(test_cc_th_create_with_attr_and_join);
    RUN_TEST(test_cc_th_detach);
    RUN_TEST(test_cc_th_self);
    RUN_TEST(test_cc_th_equal_id);
    RUN_TEST(test_cc_th_exit);

    // TLS tests
    RUN_TEST(test_cc_tls_key_create_and_delete);
    RUN_TEST(test_cc_tls_set_and_get_same_thread);
    RUN_TEST(test_cc_tls_set_and_get_multiple_threads);
    RUN_TEST(test_cc_tls_cleanup);

    return UNITY_END();
}