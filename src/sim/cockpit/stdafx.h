#if _MSC_VER < 1200
#error You need VC6 or higher
#endif // _MSC_VER < 1200

#pragma once

// Smart ptr stuff
#include <comdef.h>

// DirectX
#define D3D_OVERLOADS

#include <ddraw.h>
#include <d3d.h>
#include <d3dxcore.h>
//#include <d3dxmath.h>

// STL
#include <vector>
#include <string>
#include <map>

// Misc
#include <io.h>
#include <math.h>
#include <stdio.h>

#include "graphics/include/smart.h"
#include "falclib/include/falclib.h"
#include "dispopts.h"
#include "playerop.h"
#include "Graphics/Include/renderow.h"
#include "otwdrive.h"
