/**
 * Kevin Lin and Carter Sullivan
 * Yalnix
 * CS58 - Operating Systems 24W
 *
 * trap.h
 * 
 */

#include "globals.h"
#include <hardware.h>

void TrapKernel(UserContext *user_context);
void TrapClock(UserContext *user_context);
void TrapIllegal(UserContext *user_context);
void TrapMemory(UserContext *user_context);
void TrapMath(UserContext *user_context);
void TrapTTYReceive(UserContext *user_context);
void TrapTTYTransmit(UserContext *user_context);
void TrapDisk(UserContext *user_context);
void TrapElse(UserContext *user_context);

