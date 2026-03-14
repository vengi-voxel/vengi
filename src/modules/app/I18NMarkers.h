/**
 * @file
 * @brief Lightweight no-op translation markers that don't pull in App.h
 *
 * Use N_() and NC_() to mark strings for extraction without runtime translation.
 * Include I18N.h only in .cpp files where you need _() or C_() for actual translation.
 */

#pragma once

// no-op translation marker that still could be used for string extraction
#define N_(x) x
#define NC_(ctx, x) x
