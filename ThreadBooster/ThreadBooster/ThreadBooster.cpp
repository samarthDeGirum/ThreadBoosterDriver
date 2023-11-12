#include <ntifs.h> // Includes Kernel Thread API calls and the ntddk header file
#include "ThreadBoosterCommon.h" 

// Prototypes for Functions 
void BoosterUnloadRoutine(PDRIVER_OBJECT DriverObject);
NTSTATUS BoosterCreateCloseDispatchRtn(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS BoosterWriteDispatchRtn(PDEVICE_OBJECT DeviceObject, PIRP Irp);


// Beginning with the Driver Initialization Routine (DriverEntry) 

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	KdPrint(("ThreadBooster: Driver Entry Routine\n"));
	UNREFERENCED_PARAMETER(RegistryPath); /* Will not be using RegistryPath, silence compile-time warnings  */
	// We start by defining the Unload Routine of the Driver 
	DriverObject->DriverUnload = BoosterUnloadRoutine;
	/*
	*  Next, we need to take care of the Dispatch Routines of the Driver
	* The dispatch routines we are dealing with are CREATE, CLOSE, AND WRITE
	* In the cases of CREATE and CLOSE, we can have a common function since we will just accept
	* the I/O REQUEST PACKET (IRP) and complete the I/O Request
	* For the WRITE dispatch routine, we will be executing the main code, i.e. change the priority of the thread TID
	*/
	DriverObject->MajorFunction[IRP_MJ_CREATE] = BoosterCreateCloseDispatchRtn;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = BoosterCreateCloseDispatchRtn;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = BoosterWriteDispatchRtn; // Triggered when user mode calls WriteFile

	// After defining unload and dispatch routines, we need to create a DeviceObject for the driver and a corresponding Symbolic Link 

	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\ThreadBooster"); // Under \Device\ 
	PDEVICE_OBJECT DeviceObject; // Will be used to store the pointer to the device object created by IoCreateDevice 

	NTSTATUS status;
	status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&DeviceObject
	);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object for the ThreadBoosterDriver \n"));
		return status;
	}
	// At this point, we have created a DeviceObject, so we create a symbolic link so that user-mode can access it
	UNICODE_STRING symbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\Booster");
	status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link \n"));
		// If this creation of link has failed, we need to delete the DeviceObject 
		IoDeleteDevice(DeviceObject);
		return status;
	}
	return STATUS_SUCCESS;

}

void BoosterUnloadRoutine(PDRIVER_OBJECT DriverObject) {
	// In the unload routine of the driver, we undo whatever we did in the initialization phase
	// in this case, we first delete the symbolic link, followed by the deletion of the device object 
	KdPrint(("ThreadBooster Driver unload routine\n"));
	UNICODE_STRING symbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\Booster");
	IoDeleteSymbolicLink(&symbolicLinkName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS BoosterCreateCloseDispatchRtn(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	// IN this routine, we simply accept the IRP request, and give it a SUCCESS status, closing the IRP 
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS BoosterWriteDispatchRtn(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	auto status = STATUS_SUCCESS; 
	ULONG_PTR bytesRead = 0;

	// Go the particular stack location of this request 
	auto irpLocation = IoGetCurrentIrpStackLocation(Irp); 
	do {
		// First, we check if the buffer size we have received is at least the size of the ThreadData structure 
		if (irpLocation->Parameters.Write.Length < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		// Now, we obtain a pointer to the data (UserBuffer member of irpLocation needs to be accessed) 
		auto userData = static_cast<ThreadData*>(Irp->UserBuffer); // static_cast to a ThreadData* from a void type pointer
		// Check if the pointer to the data is not null, then confirm the priority is between (0,31) 
		if (userData == nullptr || userData->Priority < 1 || userData->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		// Now, we need to look up the thread using its ID 
		PETHREAD requestedThread; 
		status = PsLookupThreadByThreadId(ULongToHandle(userData->ThreadId), &requestedThread);
		if (!NT_SUCCESS(status)) {
			// Thread not found
			break;
		}
		// Reference to requestedThread is acquired and incremented by 4, remember to free to avoid mem leakage
		auto oldPriority = KeSetPriorityThread(requestedThread, userData->Priority);
		KdPrint(("Priority change for thread %u from %d to %d succeeded!\n",
			userData->ThreadId, oldPriority, userData->Priority));
		UNREFERENCED_PARAMETER(oldPriority);

		ObDereferenceObject(requestedThread);
		bytesRead = sizeof(ThreadData); // Report back the number of bytes used up
	} while (false);

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = bytesRead; // Reporting back this information
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

