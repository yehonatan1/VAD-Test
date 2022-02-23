#include<Ntifs.h>
#include <ntddk.h>
#include <WinDef.h>
#include <Psapi.h>
#include "Structs.h"


void SampleUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("Stopped Sample");
}
int getPIDByName(wchar_t* name) {
	PEPROCESS ep;
	if (::PsLookupProcessByProcessId(::PsGetCurrentProcessId(), &ep) == STATUS_INVALID_PARAMETER) {
		ObDereferenceObject(ep);
		DbgPrint("Can't get EPROCESS");
		return STATUS_INVALID_PARAMETER;
	}

	PUNICODE_STRING Path = NULL;
	::SeLocateProcessImageName(ep, &Path);
	PLIST_ENTRY Process_List_Entry = ((LIST_ENTRY*)((LPBYTE)ep + 0x448));
	PLIST_ENTRY List_Entry = Process_List_Entry->Flink;
	LPBYTE pUpi;
	//DbgPrint("Starting with buffer path: %wZ", Path);
	while (Path->Buffer == NULL) {
		//DbgPrint("The buffer is null so going forward to next process: %wZ", Path);
		pUpi = ((LPBYTE)List_Entry) - 0x448;
		ep = ((PEPROCESS)pUpi);
		::SeLocateProcessImageName(ep, &Path);
		List_Entry = List_Entry->Flink;
	}

	//DbgPrint("1.The path is: %wZ", Path);
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
	int UniqueProcessId = *((int*)pUpi); //Notepad PID
	//DbgPrint("The PID of %wZ is %d\n",UniqueProcessId,name);
	return UniqueProcessId;
}


extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	DbgPrint("Sample driver Load called\n");
	KdPrint(("Test"));
	DriverObject->DriverUnload = SampleUnload;
	PEPROCESS eprocess = nullptr;
	//int pid = getPIDByName(L"notepad.exe");
	//DbgPrint("The PID of notepad is: %d", pid);
	int pid = getPIDByName(L"notepad.exe");
	if (pid <= 0 || PsLookupProcessByProcessId((HANDLE)pid, &eprocess) == STATUS_INVALID_PARAMETER)
	{
		ObDereferenceObject(eprocess);
		DbgPrint("Can't get EPROCESS");
		return STATUS_INVALID_PARAMETER;
	}
	DbgPrint("The pid of notepad is %d\n", pid);
	DbgPrint("It worked");
	
	
	LPBYTE pUpi = ((LPBYTE)eprocess) + 0x7d8;
	PRTL_AVL_TREE vadRoot = nullptr;
	vadRoot = ((PRTL_AVL_TREE)pUpi);
	DbgPrint("The parent value is %llu\n", vadRoot->Root->ParentValue);

	/*
	PMMVAD pVadRoot = NULL;
	pVadRoot = ((PMMVAD)pUpi);
	DbgPrint("Test");
	pUpi = ((LPBYTE)eprocess) + 0x80;
	PFILE_OBJECT fileObject = ((PFILE_OBJECT)pUpi);
	DbgPrint("%wZ\n", fileObject->FileName);
	*/
	return STATUS_SUCCESS;
}
