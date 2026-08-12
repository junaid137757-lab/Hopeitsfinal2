#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

/* User Management module: register, login,
   forgot/change password. Owns users.dat. */

void registerUser(void);
int loginUser(void);
void forgotPassword(void);
void changePassword(void);

#endif
