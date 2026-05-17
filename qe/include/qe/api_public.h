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

#ifndef QE_API_PUBLIC_H__
#define QE_API_PUBLIC_H__

#include "api_public_impl.h"

#define QE_API(rettype)         QE_EXTERN_C_ rettype QE_CALL_
#define QE_FFI_API(rettype)     QE_EXTERN_C_ QE_EXPORT_ rettype QE_CALL_
#define QE_ALIGNAS(n)           QE_ALIGNAS_(n)

#endif // QE_API_PUBLIC_H__
