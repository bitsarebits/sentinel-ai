# Unity Testing Framework Cheatsheet (C)

Structure:
   void setUp(void) { /* Init per-test data */ }
   void tearDown(void) { /* Cleanup per-test data */ }

Assertions:
   TEST_ASSERT_EQUAL_INT(expected, actual);
   TEST_ASSERT_EQUAL_HEX8(expected, actual);
   TEST_ASSERT_EQUAL_STRING("expected", actual);
   TEST_ASSERT_NULL(pointer);
   TEST_ASSERT_NOT_NULL(pointer);

Running Tests:
   int main(void) {
       UNITY_BEGIN();
       RUN_TEST(test_function_name);
       return UNITY_END();
   }
