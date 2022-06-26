#include<Ntifs.h>
#include <ntddk.h>
#include <WinDef.h>
#include "structs.h"



typedef struct _VAD_INFO
{
	LONG level;
	//ULONG pAddress;
	PFILE_OBJECT pFileObject;
	PUNICODE_STRING Name;
}VAD_INFO, * PVAD_INFO;

PEPROCESS getEprocessByName(wchar_t* name);
PRTL_BALANCED_NODE GetVadRoot(wchar_t* szProcessName);
VOID ListVad(PRTL_BALANCED_NODE pParentVad, int level);
VOID SetVadInfo(PMMVAD_SHORT pVad, PVAD_INFO pVadInfo);
VOID DisplayVadInfo(PVAD_INFO pVadInfo);
int getPIDByName(wchar_t* name);
PFILE_OBJECT MMVADToFileObject(MMVAD node);



void SampleUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("Sample driver Unload called\n"));
}

PFILE_OBJECT MMVADToFileObject(PMMVAD node) {
	LPBYTE byte = ((LPBYTE)node) + 0x80;
	return (PFILE_OBJECT)byte;
}



VOID SetVadInfo(PMMVAD_SHORT pVad, PVAD_INFO pVadInfo) {
	//pVadInfo->pAddress = (ULONG)pVad;
	LPBYTE byte = ((LPBYTE)pVad+0x80);
	pVadInfo->pFileObject = ((PFILE_OBJECT)byte);
	pVadInfo->Name = &pVadInfo->pFileObject->FileName;
}

VOID DisplayVadInfo(PVAD_INFO pVadInfo)
{
	//DbgPrint("\n[+]0x%x", pVadInfo->pAddress);
	KdPrint(("      Level : %ld", pVadInfo->level));
	//KdPrint(("      File Object : 0x%x", pVadInfo->pFileObject));
	KdPrint(("      Name : %wZ", pVadInfo->Name));
	return;
}


PRTL_BALANCED_NODE GetVadRoot(wchar_t* szProcessName) {
	//UNREFERENCED_PARAMETER(szProcessName);
	KdPrint(("Get Vad Root\n"));
	PEPROCESS ep = NULL;
	PRTL_BALANCED_NODE pVadRoot = NULL;
	LPBYTE byte;
	int x = getPIDByName(szProcessName);
	
	PsLookupProcessByProcessId((HANDLE)x,&ep);
	if (ep == 0x0) {
		ObDereferenceObject(ep);
		KdPrint(("Not Working\n"));
		return 0x0;
	}
	byte = ((LPBYTE)ep) + 0x7d8;
	pVadRoot = ((PRTL_AVL_TREE)byte)->Root;
	//DbgPrint("[+]0x%p\n", pVadRoot);
	return pVadRoot;
}



//Getting process pid by his name
int getPIDByName(wchar_t* name) {
	PEPROCESS ep;

	//Getting the Eprocess structure of the current process
	if (::PsLookupProcessByProcessId(::PsGetCurrentProcessId(), &ep) == STATUS_INVALID_PARAMETER) {
		ObDereferenceObject(ep);
		DbgPrint("Can't get EPROCESS");
		return STATUS_INVALID_PARAMETER;
	}


	PUNICODE_STRING Path = NULL;
	//Getting the image name
	::SeLocateProcessImageName(ep, &Path);

	//Getting the ActiveProcessLinks of the process
	PLIST_ENTRY Process_List_Entry = ((LIST_ENTRY*)((LPBYTE)ep + 0x448));
	PLIST_ENTRY List_Entry = Process_List_Entry->Flink;
	LPBYTE pUpi;
	//DbgPrint("Starting with buffer path: %wZ", Path);

	//Skiping all the process with null image name
	while (Path->Buffer == NULL) {
		//DbgPrint("The buffer is null so going forward to next process: %wZ", Path);
		//Getting the EPROCESS of ActiveProcessLinks
		pUpi = ((LPBYTE)List_Entry) - 0x448;
		ep = ((PEPROCESS)pUpi);
		::SeLocateProcessImageName(ep, &Path);
		List_Entry = List_Entry->Flink;
	}

	//DbgPrint("1.The path is: %wZ", Path);
	//Searching the process by his name 
	while (wcsstr(Path->Buffer, name) == NULL && Process_List_Entry != List_Entry->Flink) {
		pUpi = ((LPBYTE)List_Entry) - 0x448;
		ep = ((PEPROCESS)pUpi);
		::SeLocateProcessImageName(ep, &Path);
		//DbgPrint("2. The path is: %wZ", (const wchar_t*)Path);
		List_Entry = List_Entry->Flink;
	}
	if (Process_List_Entry == List_Entry->Flink) {
		//DbgPrint("%wZ isn't running quiting!", name);
		return STATUS_SUCCESS;
	}
	pUpi = ((LPBYTE)List_Entry->Blink) - 0x448 + 0x440;
	int UniqueProcessId = *((int*)pUpi); //Process PID
	DbgPrint("The PID of %ls is %d\n", name, UniqueProcessId);
	return UniqueProcessId;
}

VOID ListVad(PRTL_BALANCED_NODE pParentVad,int level) {


	if (pParentVad == NULL || !MmIsAddressValid(pParentVad))
		return;

	UNREFERENCED_PARAMETER(level);
	PRTL_BALANCED_NODE pVadLeft = NULL;
	PRTL_BALANCED_NODE pVadRight = NULL;
	PFILE_OBJECT fileObject = NULL;
	pVadLeft = pParentVad->Left;
	pVadRight = pParentVad->Right;
	fileObject = MMVADToFileObject((PMMVAD)((LPBYTE)pParentVad));

	if (MmIsAddressValid((PULONG)fileObject) == FALSE)
		DbgPrint("Address isn't valid");

	DbgPrint("[+]0x%p %d\n", pParentVad,level);
	//DbgPrint("\tName: %wZ\n", MMVADToFileObject((PMMVAD)((LPBYTE)pParentVad))->FileName);
	//DbgPrint("Test\n");





	pVadLeft = pParentVad->Left;
	pVadRight = pParentVad->Right;
	
	if (MmIsAddressValid(pVadLeft))
		ListVad(pVadLeft, level + 1);

	if (MmIsAddressValid(pVadRight))
		ListVad(pVadRight, level + 1);
	
	return;
}



extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = SampleUnload;
	KdPrint(("Sample driver Load called\n"));
	wchar_t* str = L"Calculator.exe";
	ListVad(GetVadRoot(str),1);
	DbgPrint("Test Main\n");

	return STATUS_SUCCESS;
}
