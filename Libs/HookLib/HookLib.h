#pragma once

#include <map>
#include <vector>

/**
*	\class HookLib
*	\brief HookLib singleton class
**/
struct HookLib
{
	/**
	*	\brief Initialize
	*
	*	Initialize HookLib. Must be called before doing anything.
	*
	**/
	static void Initialize();

	/**
	*	\brief DetourFunction
	*
	*	Detour a target function. The minimum of nLength is 5 byte,
	*	if nLength is less than 5 byte it will fail to detour a target function.
	*	If the method succeeds, the return value is return address to original function.
	*	If the method fails, the return value is NULL.
	*
	*	\return return address to source function (target function)
	*	\param pSrc Source function (target function)
	*	\param pDest Detour function
	*	\param nLength The length of instrunction (must be 5 bytes or above)
	**/
	static uint8_t* DetourFunction(uint8_t* const pSrc, const uint8_t* pDest, int nLength);

	/**
	*	\brief DetourMemory
	*
	*	Detour a function manually from memory. The minimum of nLength is 5 byte,
	*	if nLength is less than 5 byte it will fail to detour a target address.
	*	If the method succeeds, the return value is return address to source address.
	*	If the method fails, the return value is NULL.
	*
	*	\return return address to source address
	*	\param uptrTargetAddr Source address
	*	\param pDestAddr Detour address
	*	\param nLength The length of instrunction (must be 5 bytes or above)
	**/
	static uint8_t* DetourMemory(uintptr_t uptrTargetAddr, void* pDestAddr, int nLength);

	/**
	*	\brief Shutdown
	*
	*	Shutdown and release all resources used by HookLib.
	*
	**/
	static void Shutdown();

	/**
	*	HookLib utilities
	**/
	struct Utils
	{
		/**
		*	\brief GetVMTPointer
		*
		*	Get a pointer to virtual method table.
		*
		*	\param pObject Pointer to object
		*	\param ppOutVMTPtr Result
		**/
		static void GetVMTPointer(const void* pObject, uintptr_t*& ppOutVMTPtr);

		/**
		*	\brief GetVMTPointer
		*
		*	Get a virtual method table as std::vector.
		*	You must specify how many virtual methods you want to add.
		*
		*	\param pObject Pointer to object
		*	\param num Number of virtual methods.
		*	\param outVMTVector Result as std::vector
		**/
		static void GetVMTVector(const void* pObject, int num, std::vector<uintptr_t>& outVMTVector);
	};

private:
	static std::map<uintptr_t, uintptr_t>* m_pHookTable;
	static bool m_bInitialized;

private:
	static int CheckExistingHook();
	static void DeleteAllHook();
};