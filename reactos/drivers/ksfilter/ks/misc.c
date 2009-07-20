/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/misc.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePool(PoolType, NumberOfBytes);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{

    ExFreePool(Item);
}

NTSTATUS
NTAPI
KspForwardIrpSynchronousCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    if (Irp->PendingReturned == TRUE)
    {
        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
KspForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* initialize the notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, KspForwardIrpSynchronousCompletion, (PVOID)&Event, TRUE, TRUE, TRUE);

    /* now call the driver */
    Status = IoCallDriver(DeviceHeader->NextDeviceObject, Irp);
    /* did the request complete yet */
    if (Status == STATUS_PENDING)
    {
        /* not yet, lets wait a bit */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersectionEx(
    IN  PIRP Irp,
    IN  PKSP_PIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  ULONG DescriptorSize,
    IN  PFNKSINTERSECTHANDLEREX IntersectHandler OPTIONAL,
    IN  PVOID HandlerContext OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
