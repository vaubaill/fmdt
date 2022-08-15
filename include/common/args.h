// ------------------
// --- Args.h ---
// ------------------

#pragma once

void args_del_arg(int argc, char** argv, int index);
int args_find_arg(int argc, char* argv[], const char* arg);
int args_find_int_arg(int argc, char** argv, const char* arg, int def);
float args_find_float_arg(int argc, char** argv, const char* arg, float def);
char* args_find_char_arg(int argc, char** argv, const char* arg, char* def);
