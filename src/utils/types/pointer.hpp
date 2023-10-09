#pragma once

/**
 * A const pointer declaration indicating that this pointer is explicitly non-owning.
 * There is no guarantee that this pointer is pointing to valid memory.
 * Modifications to the underlying object are not allowed.
 */
template<typename T>
using observer_ptr = const T*;
