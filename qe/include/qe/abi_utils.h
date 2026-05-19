/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE_ABI_UTILS_H
#define QE_ABI_UTILS_H

#include <qe/internals/abi_defs.h>


#define QE_FFI_API(rettype)     QE_FFI_API_(rettype)
#define QE_IMPORT(module, name) QE_IMPORT_(module, name)


#endif // QE_ABI_UTILS_H
