#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "authentication.h"
#include "common.h"

/* users.dat lives in the working directory, so every test in this
   suite gets its own scratch temp dir. */
static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

/* A valid username plus a password meeting all four requirements
   (upper, lower, digit, symbol, 8+ chars) should register cleanly
   on the first try. */
static void test_register_valid_user(void)
{
    th_reset_globals();

    th_feed_stdin("alice\nGoodPass1!\n");
    th_capture_stdout_start();
    registerUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Account Created Successfully") != NULL);
    free(out);
}

/* Registering the same username twice must be rejected the
   second time. */
static void test_register_duplicate_rejected(void)
{
    th_reset_globals();

    th_feed_stdin("bob\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_feed_stdin("bob\nOtherPass2@\n");
    th_capture_stdout_start();
    registerUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Username Already Exists") != NULL);
    free(out);
}

/* A password that's too weak should be rejected and re-prompted;
   once a valid password follows, registration should succeed. */
static void test_register_weak_password_then_valid(void)
{
    th_reset_globals();

    th_feed_stdin("carol\nweak\nGoodPass1!\n");
    th_capture_stdout_start();
    registerUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "does not meet the requirements") != NULL);
    CU_ASSERT(strstr(out, "Account Created Successfully") != NULL);
    free(out);
}

/* Entering "0" for the username should cancel registration
   immediately. */
static void test_register_cancel_via_username(void)
{
    th_reset_globals();

    th_feed_stdin("0\n");
    th_capture_stdout_start();
    registerUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Registration Cancelled") != NULL);
    free(out);
}

/* Entering "0" for the password (after a valid, new username)
   should also cancel registration. */
static void test_register_cancel_via_password(void)
{
    th_reset_globals();

    th_feed_stdin("dave\n0\n");
    th_capture_stdout_start();
    registerUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Registration Cancelled") != NULL);
    free(out);
}

/* Logging in with the exact username/password just registered
   should succeed, return 1, and set currentUser. */
static void test_login_success_sets_current_user(void)
{
    th_reset_globals();

    th_feed_stdin("erin\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_reset_globals();
    th_feed_stdin("erin\nGoodPass1!\n");
    th_capture_stdout_start();
    int result = loginUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(result, 1);
    CU_ASSERT_STRING_EQUAL(currentUser, "erin");
    CU_ASSERT(strstr(out, "Login Successful") != NULL);
    free(out);
}

/* Logging in with a wrong password should return 0 and not set
   currentUser. */
static void test_login_wrong_password(void)
{
    th_reset_globals();

    th_feed_stdin("frank\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_reset_globals();
    th_feed_stdin("frank\nWrongPass9#\n");
    th_capture_stdout_start();
    int result = loginUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT(strstr(out, "Invalid Username or Password") != NULL);
    free(out);
}

/* Logging in when no users.dat exists at all should return -1
   with a helpful message rather than crashing. */
static void test_login_no_users_registered(void)
{
    th_enter_tmp_dir(); /* extra-fresh, empty dir, no users.dat */
    th_reset_globals();

    th_capture_stdout_start();
    int result = loginUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(result, -1);
    CU_ASSERT(strstr(out, "No Users Registered Yet") != NULL);
    free(out);

    th_leave_tmp_dir();
}

/* Entering "0" for the username during login should cancel and
   return -1. */
static void test_login_cancel(void)
{
    th_reset_globals();

    th_feed_stdin("gina\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_reset_globals();
    th_feed_stdin("0\n");
    th_capture_stdout_start();
    int result = loginUser();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(result, -1);
    CU_ASSERT(strstr(out, "Login Cancelled") != NULL);
    free(out);
}

/* forgotPassword() should let a known user set a new password,
   after which the OLD password no longer logs in and the NEW one
   does. */
static void test_forgot_password_resets_and_relogs_in(void)
{
    th_reset_globals();

    th_feed_stdin("hank\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_reset_globals();
    th_feed_stdin("hank\nNewPass2@\n");
    th_capture_stdout_start();
    forgotPassword();
    char *out = th_capture_stdout_end();
    CU_ASSERT(strstr(out, "Password Reset Successfully") != NULL);
    free(out);

    th_reset_globals();
    th_feed_stdin("hank\nGoodPass1!\n");
    int oldResult = loginUser();
    CU_ASSERT_EQUAL(oldResult, 0);

    th_reset_globals();
    th_feed_stdin("hank\nNewPass2@\n");
    int newResult = loginUser();
    CU_ASSERT_EQUAL(newResult, 1);
}

/* forgotPassword() for a username that was never registered
   should report "not found" and change nothing. */
static void test_forgot_password_unknown_user(void)
{
    th_reset_globals();

    th_feed_stdin("ghost\n");
    th_capture_stdout_start();
    forgotPassword();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Username Not Found") != NULL);
    free(out);
}

/* changePassword() with the correct current password should
   succeed, after which login requires the NEW password. */
static void test_change_password_success(void)
{
    th_reset_globals();

    th_feed_stdin("ivy\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_reset_globals();
    th_feed_stdin("ivy\nGoodPass1!\n");
    th_capture_stdout_start();
    loginUser(); /* sets currentUser = "ivy" */
    free(th_capture_stdout_end());

    th_feed_stdin("GoodPass1!\nNewPass3#\n");
    th_capture_stdout_start();
    changePassword();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Password Changed Successfully") != NULL);
    free(out);

    th_reset_globals();
    th_feed_stdin("ivy\nNewPass3#\n");
    int result = loginUser();
    CU_ASSERT_EQUAL(result, 1);
}

/* changePassword() with an incorrect current password should be
   rejected and must not alter the stored password. */
static void test_change_password_wrong_old_password(void)
{
    th_reset_globals();

    th_feed_stdin("jack\nGoodPass1!\n");
    free(th_call_capturing(registerUser));

    th_reset_globals();
    th_feed_stdin("jack\nGoodPass1!\n");
    th_capture_stdout_start();
    loginUser();
    free(th_capture_stdout_end());
    th_capture_stdout_start();
    changePassword();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Incorrect Password") != NULL);
    free(out);
}

void add_authentication_suite(void)
{
    CU_pSuite suite = CU_add_suite("authentication", suite_init, suite_clean);

    CU_add_test(suite, "register a valid new user", test_register_valid_user);
    CU_add_test(suite, "register rejects a duplicate username", test_register_duplicate_rejected);
    CU_add_test(suite, "register re-prompts on a weak password", test_register_weak_password_then_valid);
    CU_add_test(suite, "register cancels via username '0'", test_register_cancel_via_username);
    CU_add_test(suite, "register cancels via password '0'", test_register_cancel_via_password);
    CU_add_test(suite, "login succeeds and sets currentUser", test_login_success_sets_current_user);
    CU_add_test(suite, "login fails on wrong password", test_login_wrong_password);
    CU_add_test(suite, "login with no users registered yet", test_login_no_users_registered);
    CU_add_test(suite, "login cancels via '0'", test_login_cancel);
    CU_add_test(suite, "forgotPassword resets and re-logs in with new password", test_forgot_password_resets_and_relogs_in);
    CU_add_test(suite, "forgotPassword reports unknown username", test_forgot_password_unknown_user);
    CU_add_test(suite, "changePassword succeeds with correct old password", test_change_password_success);
    CU_add_test(suite, "changePassword rejects wrong old password", test_change_password_wrong_old_password);
}
