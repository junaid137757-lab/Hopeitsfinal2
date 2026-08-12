#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "authentication.h"
#include "common.h"
#include "utility.h"
#include "logger.h"
#include "activity_log.h"
#include "sha256.h"

/* =====================================================
   User Management module: register, login, forgot/change
   password. Owns data/users.dat.

   Password storage: only a per-user random salt and the
   SHA-256 hash of (salt || password) are ever written to
   disk or held for comparison - see sha256.h. Plaintext
   passwords exist only transiently in local stack buffers
   while the user is typing them and are never copied into
   the User record.

   MISRA-C:2012 conventions applied throughout this file:
     Rule 12.1 - every mixed operator expression is fully
                 parenthesised.
     Rule 12.3 - one declaration per line.
     Rule 15.5 - every function has a single point of exit;
                 a `result`/`status` variable is set and
                 control falls through to one final return.
     Rule 15.6 - every if/else/for/while body is a braced
                 compound statement, even one-liners.
     Rule 17.7 - every discarded return value is cast (void).
   ===================================================== */

/* Checks: at least 8 characters, with at least one uppercase,
   one lowercase, one digit, and one symbol. Not exposed outside
   this file - only registerUser/forgotPassword/changePassword
   need it. */
static int isPasswordValid(const char *password)
{
    int result;
    size_t len = strlen(password);

    if(len < 8U)
    {
        result = 0;
    }
    else
    {
        int hasUpper = 0;
        int hasLower = 0;
        int hasDigit = 0;
        int hasSymbol = 0;
        size_t i;

        for(i = 0U; i < len; i++)
        {
            unsigned char c = (unsigned char)password[i];

            if(isupper(c) != 0)
            {
                hasUpper = 1;
            }
            else if(islower(c) != 0)
            {
                hasLower = 1;
            }
            else if(isdigit(c) != 0)
            {
                hasDigit = 1;
            }
            else if(ispunct(c) != 0)
            {
                hasSymbol = 1;
            }
            else
            {
                /* not a character the password strength check cares about */
            }
        }

        result = ((hasUpper != 0) && (hasLower != 0) && (hasDigit != 0) && (hasSymbol != 0)) ? 1 : 0;
    }

    return result;
}

static void printPasswordRequirements(void)
{
    (void)printf("\n----------------------------------------\n");
    (void)printf(" Password Requirements\n");
    (void)printf("----------------------------------------\n");
    (void)printf(" - At least 8 characters\n");
    (void)printf(" - At least 1 uppercase letter (A-Z)\n");
    (void)printf(" - At least 1 lowercase letter (a-z)\n");
    (void)printf(" - At least 1 digit (0-9)\n");
    (void)printf(" - At least 1 symbol (e.g. ! @ # $ %%)\n");
    (void)printf("----------------------------------------\n\n");
}

/* Prompts in a loop until a valid new password is entered, hashes
   it with a freshly-generated salt, and writes salt/hash into
   *user. Returns 1 on success, 0 if the user typed "0" to cancel
   or the salt generator failed (e.g. /dev/urandom unavailable). */
static int promptAndHashNewPassword(User *user)
{
    int status = 0;
    int done = 0;
    char plainPassword[30];

    printPasswordRequirements();

    while(done == 0)
    {
        (void)printf("Enter New Password (or 0 to cancel): ");
        readLine(plainPassword, (int)sizeof(plainPassword));

        if(strcmp(plainPassword, "0") == 0)
        {
            done = 1;
        }
        else if(isPasswordValid(plainPassword) == 0)
        {
            (void)printf("Password does not meet the requirements. Please try again.\n");
        }
        else
        {
            if(sha256GenerateSaltHex(user->saltHex) == 0)
            {
                LOG_ERROR_MSG("failed to generate password salt (RNG source unavailable)");
                (void)printf("Could not securely generate a password right now. Please try again.\n");
            }
            else
            {
                sha256HashSalted(user->saltHex, plainPassword, user->passwordHash);
                status = 1;
                done = 1;
            }
        }
    }

    /* Do not let the plaintext password linger on the stack any
       longer than necessary. */
    (void)memset(plainPassword, 0, sizeof(plainPassword));

    return status;
}

void registerUser(void)
{
    User user;
    User temp;
    int cancelled = 0;
    FILE *fp = NULL;

    (void)printf("Enter Username (or 0 to cancel): ");
    user.username[0] = '\0';
    readLine(user.username, (int)sizeof(user.username));

    if(strcmp(user.username, "0") == 0)
    {
        (void)printf("Registration Cancelled.\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        fp = fopen("data/users.dat", "ab+");

        if(fp == NULL)
        {
            LOG_ERROR_MSG("could not open data/users.dat for registration");
            (void)printf("Error Opening Users File\n");
        }
        else
        {
            int usernameTaken = 0;

            rewind(fp);

            while((usernameTaken == 0) && (fread(&temp, sizeof(User), 1, fp) == 1U))
            {
                if(strcmp(temp.username, user.username) == 0)
                {
                    usernameTaken = 1;
                }
            }

            if(usernameTaken != 0)
            {
                (void)printf("Username Already Exists!\n");
            }
            else if(promptAndHashNewPassword(&user) == 0)
            {
                (void)printf("Registration Cancelled.\n");
            }
            else
            {
                char filename[60];
                FILE *userFile;

                /* fp is an update-mode ("ab+") stream and the loop
                   above just read from it - the C standard requires
                   a positioning call (fseek/fsetpos/rewind) between
                   a read and a following write on the same
                   update-mode stream, or behaviour is undefined.
                   A zero-offset SEEK_CUR seek satisfies that without
                   actually moving the position (append mode always
                   writes at EOF regardless). */
                (void)fseek(fp, 0, SEEK_CUR);
                (void)fwrite(&user, sizeof(User), 1, fp);

                (void)snprintf(filename, sizeof(filename), "data/%s_transactions.dat", user.username);

                userFile = fopen(filename, "wb");

                if(userFile != NULL)
                {
                    int count = 0;

                    (void)fwrite(&count, sizeof(int), 1, userFile);
                    (void)fclose(userFile);
                }

                (void)printf("Account Created Successfully!\n");
            }

            (void)fclose(fp);
        }
    }
}

int loginUser(void)
{
    User user;
    char username[30];
    char password[30];
    int result = 0;
    int cancelled = 0;
    FILE *fp = fopen("data/users.dat", "rb");

    if(fp == NULL)
    {
        LOG_ERROR_MSG("failed to open data/users.dat - no users registered yet");
        (void)printf("\nNo Users Registered Yet!\n");
        (void)printf("Please Register First.\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Username (or 0 to cancel): ");
        readLine(username, (int)sizeof(username));

        if(strcmp(username, "0") == 0)
        {
            (void)printf("Login Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Password (or 0 to cancel): ");
        readLine(password, (int)sizeof(password));

        if(strcmp(password, "0") == 0)
        {
            (void)printf("Login Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        int found = 0;

        while((found == 0) && (fread(&user, sizeof(User), 1, fp) == 1U))
        {
            if(strcmp(user.username, username) == 0)
            {
                char candidateHash[SHA256_HEX_CHARS];

                sha256HashSalted(user.saltHex, password, candidateHash);

                if(sha256ConstantTimeEqual(candidateHash, user.passwordHash) != 0)
                {
                    found = 1;
                }
            }
        }

        if(found != 0)
        {
            char logMsg[80];

            (void)strcpy(currentUser, username);
            (void)snprintf(logMsg, sizeof(logMsg), "user '%s' logged in successfully", username);
            LOG_INFO_MSG(logMsg);
            (void)printf("Login Successful!\n");
            result = 1;
        }
        else
        {
            char logMsg[80];

            (void)snprintf(logMsg, sizeof(logMsg), "failed login attempt for username '%s'", username);
            LOG_WARN_MSG(logMsg);
            (void)printf("Invalid Username or Password!\n");
            result = 0;
        }
    }

    if(fp != NULL)
    {
        (void)fclose(fp);
    }

    if(cancelled != 0)
    {
        result = -1;
    }

    /* Do not let the plaintext password linger on the stack any
       longer than necessary. */
    (void)memset(password, 0, sizeof(password));

    return result;
}

void forgotPassword(void)
{
    User user;
    char username[30];
    int cancelled = 0;
    FILE *fp = fopen("data/users.dat", "rb+");

    if(fp == NULL)
    {
        (void)printf("\nNo Users Registered Yet!\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Username (or 0 to cancel): ");
        readLine(username, (int)sizeof(username));

        if(strcmp(username, "0") == 0)
        {
            (void)printf("Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        int found = 0;

        while((found == 0) && (fread(&user, sizeof(User), 1, fp) == 1U))
        {
            if(strcmp(user.username, username) == 0)
            {
                found = 1;
            }
        }

        if(found == 0)
        {
            (void)printf("Username Not Found!\n");
        }
        else if(promptAndHashNewPassword(&user) == 0)
        {
            (void)printf("Cancelled.\n");
        }
        else
        {
            (void)fseek(fp, -(long)sizeof(User), SEEK_CUR);
            (void)fwrite(&user, sizeof(User), 1, fp);
            (void)printf("Password Reset Successfully! You can now log in.\n");
        }
    }

    if(fp != NULL)
    {
        (void)fclose(fp);
    }
}

void changePassword(void)
{
    User user;
    char oldPassword[30];
    int cancelled = 0;
    FILE *fp = fopen("data/users.dat", "rb+");

    if(fp == NULL)
    {
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        int found = 0;

        while((found == 0) && (fread(&user, sizeof(User), 1, fp) == 1U))
        {
            if(strcmp(user.username, currentUser) == 0)
            {
                found = 1;
            }
        }

        if(found == 0)
        {
            (void)printf("Account Record Not Found!\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Current Password (or 0 to cancel): ");
        readLine(oldPassword, (int)sizeof(oldPassword));

        if(strcmp(oldPassword, "0") == 0)
        {
            (void)printf("Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        char candidateHash[SHA256_HEX_CHARS];

        sha256HashSalted(user.saltHex, oldPassword, candidateHash);

        if(sha256ConstantTimeEqual(candidateHash, user.passwordHash) == 0)
        {
            char logMsg[80];

            (void)snprintf(logMsg, sizeof(logMsg), "incorrect current password entered by '%s'", currentUser);
            LOG_WARN_MSG(logMsg);
            (void)printf("Incorrect Password!\n");
            cancelled = 1;
        }
    }

    (void)memset(oldPassword, 0, sizeof(oldPassword));

    if(cancelled == 0)
    {
        if(promptAndHashNewPassword(&user) == 0)
        {
            (void)printf("Cancelled.\n");
        }
        else
        {
            char logMsg[80];

            (void)fseek(fp, -(long)sizeof(User), SEEK_CUR);
            (void)fwrite(&user, sizeof(User), 1, fp);
            (void)printf("Password Changed Successfully!\n");
            logActivity("Changed Password");
            (void)snprintf(logMsg, sizeof(logMsg), "user '%s' changed their password", currentUser);
            LOG_INFO_MSG(logMsg);
        }
    }

    if(fp != NULL)
    {
        (void)fclose(fp);
    }
}
