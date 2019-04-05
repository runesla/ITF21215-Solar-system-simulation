#pragma once
#ifndef READFILE_H
#define READFILE_H

#include <stdlib.h>
#include <stdio.h>

// Utility function for reading file from disk
char *readSourceFile(const char *filename, int *size);

#endif