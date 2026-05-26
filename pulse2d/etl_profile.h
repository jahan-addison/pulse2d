/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

// ETL_NO_EXCEPTIONS: use error handler callbacks instead of throw.
// Required when building with -fno-exceptions (Teensyduino default).
// With ETL_THROW_EXCEPTIONS + -fno-exceptions, throw compiles to
// std::terminate() with no message — ETL errors are completely silent.
#define ETL_NO_EXCEPTIONS
#define ETL_VERBOSE_ERRORS
#define ETL_CHECK_PUSH_POP
#define ETL_LOG_ERRORS