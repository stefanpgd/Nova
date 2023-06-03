#pragma once

#if defined(max)
#undef max
#endif

typedef long HRESULT;
void ThrowIfFailed(const HRESULT hr);