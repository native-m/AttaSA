#include <Windows.h>
#include <process.h>
#include <d3d9.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "resource.h"

#define STB_IMAGE_IMPLEMENTATION

#pragma comment(lib, "d3d9.lib")

#ifndef _DEBUG
#pragma comment(lib, "HookLib/HookLib.lib")
#else
#pragma comment(lib, "HookLib/HookLib_debug.lib")
#endif

#include "../Libs/HookLib/HookLib.h"
#include "../Libs/stb/stb_image.h"

typedef void(__stdcall *D3D9DrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT); // 82

HINSTANCE hDllInstance;

// Function return
D3D9DrawIndexedPrimitive g_pD3D9DrawIndexedPrimitive = nullptr;
uintptr_t g_pD3D9BeginScene = 0;

// D3D9 vtable
uintptr_t* g_pD3D9Vtable = nullptr;

// flag
bool g_bTextureInitialized = false;

// Atta texture here
IDirect3DTexture9* g_pAttaTexture = nullptr;

unsigned __stdcall Main(void* param);
void Initialize();
bool InitD3D();
void __stdcall Hook_D3D9DrawIndexedPrimitive(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount);
void Invoker_D3D9BeginScene();
void MainHook_D3D9BeginScene_InitTexture(IDirect3DDevice9* pDevice);
void ReleaseResource();

template <class ComType>
void ReleaseCom(ComType** pComObject)
{
	if (*pComObject)
	{
		(*pComObject)->Release();
		*pComObject = nullptr;
	}
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	hDllInstance = hInstance;
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInstance);
		HookLib::Initialize();
		_beginthreadex(nullptr, 0, Main, nullptr, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		ReleaseResource();
		HookLib::Shutdown();
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}

	return TRUE;
}

unsigned __stdcall Main(void* param)
{
	Initialize();
	return 0;
}

void Initialize()
{
	if (!InitD3D())
		MessageBox(nullptr, L"Failed to create d3d", L"ERROR", MB_OK | MB_ICONERROR);
}

bool InitD3D()
{
	HRESULT hr;
	IDirect3D9* pD3D9;
	IDirect3DDevice9* pDevice;
	D3DPRESENT_PARAMETERS d3dPresentParameters;
	HWND hWnd = FindWindow(nullptr, L"GTA: San Andreas");

	pD3D9 = Direct3DCreate9(D3D9b_SDK_VERSION);
	if (!pD3D9)
		return false;

	ZeroMemory(&d3dPresentParameters, sizeof(d3dPresentParameters));
	d3dPresentParameters.Windowed = true;
	d3dPresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParameters.hDeviceWindow = hWnd;
	d3dPresentParameters.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dPresentParameters.BackBufferHeight = 640;
	d3dPresentParameters.BackBufferWidth = 480;
	d3dPresentParameters.EnableAutoDepthStencil = TRUE;
	d3dPresentParameters.AutoDepthStencilFormat = D3DFMT_D16;

	hr = pD3D9->CreateDevice(0, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPresentParameters, &pDevice);
	if (FAILED(hr))
		return false;

	HookLib::Utils::GetVMTPointer(pDevice, g_pD3D9Vtable);
	g_pD3D9DrawIndexedPrimitive = (D3D9DrawIndexedPrimitive)HookLib::DetourFunction((uint8_t*)g_pD3D9Vtable[82], (uint8_t*)Hook_D3D9DrawIndexedPrimitive, 5);
	g_pD3D9BeginScene = (uintptr_t)HookLib::DetourMemory(g_pD3D9Vtable[41] + 0xD0, Invoker_D3D9BeginScene, 6);

	ReleaseCom(&pDevice);
	ReleaseCom(&pD3D9);
	return true;
}

void __stdcall Hook_D3D9DrawIndexedPrimitive(IDirect3DDevice9 * pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount)
{
	if (NumVertices > 24 && g_pAttaTexture != nullptr)
	{
		pDevice->SetTexture(0, g_pAttaTexture);
	}

	return g_pD3D9DrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
}

__declspec(naked)
void Invoker_D3D9BeginScene()
{
	__asm
	{
		// 0x10053F20
		or [edx + 2BFCh], edi
		pushad
	}

	// invoke the main hook
	__asm
	{
		push[ebp + 8]
		call MainHook_D3D9BeginScene_InitTexture
		add esp, 4
	}

	__asm
	{
		popad
		jmp g_pD3D9BeginScene
	}
}

void MainHook_D3D9BeginScene_InitTexture(IDirect3DDevice9 * pDevice)
{
	HRESULT hr;
	HRSRC hResource;
	HGLOBAL hResData;
	DWORD dwDataSize;
	LPVOID lpResAddr;
	unsigned char* img;
	int w, h, n;

	if (g_bTextureInitialized)
		return;

	hResource = FindResource(hDllInstance, MAKEINTRESOURCE(IDB_ATTATEXTURE), L"PNG");
	hResData = LoadResource(hDllInstance, hResource);
	dwDataSize = SizeofResource(hDllInstance, hResource);
	lpResAddr = LockResource(hResData);

	img = stbi_load_from_memory((const stbi_uc*)lpResAddr, dwDataSize, &w, &h, &n, STBI_rgb_alpha);

	hr = pDevice->CreateTexture(w, h, 1, 0, D3DFMT_X8B8G8R8, D3DPOOL_MANAGED, &g_pAttaTexture, nullptr);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to create atta texture", L"error", MB_OK | MB_ICONERROR);
		stbi_image_free(img);
		g_bTextureInitialized = true;
		return;
	}

	D3DLOCKED_RECT rect;
	hr = g_pAttaTexture->LockRect(0, &rect, nullptr, D3DLOCK_DISCARD);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to create atta texture", L"error", MB_OK | MB_ICONERROR);
		stbi_image_free(img);
		g_bTextureInitialized = true;
		return;
	}

	memcpy_s(rect.pBits, w * h * sizeof(stbi_uc) * n, img, w * h * sizeof(stbi_uc) * n);

	hr = g_pAttaTexture->UnlockRect(0);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to create atta texture", L"error", MB_OK | MB_ICONERROR);
		stbi_image_free(img);
		g_bTextureInitialized = true;
		return;
	}

	stbi_image_free(img);
	g_bTextureInitialized = true;
}

void ReleaseResource()
{
	ReleaseCom(&g_pAttaTexture);
}
