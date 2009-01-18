/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/ntuser/csr.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <w32k.h>

//#define NDEBUG
#include <debug.h>

static HANDLE WindowsApiPort = NULL;
PEPROCESS CsrProcess = NULL;

NTSTATUS FASTCALL
CsrInit(void)
{
   NTSTATUS Status;
   UNICODE_STRING PortName;
   ULONG ConnectInfoLength;
   SECURITY_QUALITY_OF_SERVICE Qos;

   DPRINT("CsrInit started\n");
   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   ConnectInfoLength = 0;
   DPRINT("\n");
   Qos.Length = sizeof(Qos);
   Qos.ImpersonationLevel = SecurityDelegation;
   Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
   Qos.EffectiveOnly = FALSE;
   Status = ZwConnectPort(&WindowsApiPort,
                          &PortName,
                          &Qos,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &ConnectInfoLength);
   DPRINT("\n");
   if (! NT_SUCCESS(Status))
   {
      DPRINT("%x\n", Status);
      return Status;
   }

   CsrProcess = PsGetCurrentProcess();

   DPRINT("CsrInit done (process %x)\n", CsrProcess);
   return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
co_CsrNotify(PCSR_API_MESSAGE Request)
{
   NTSTATUS Status;
   PEPROCESS OldProcess;

   DPRINT("co_CsrNotify\n");

   if (NULL == CsrProcess)
   {
      return STATUS_INVALID_PORT_HANDLE;
   }

   Request->Header.u2.ZeroInit = 0;
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);

   /* Switch to the process in which the WindowsApiPort handle is valid */
   OldProcess = PsGetCurrentProcess();
   if (CsrProcess != OldProcess)
   {
      DPRINT("CsrProcess->Pcb %x\n", &CsrProcess->Pcb);
      KeAttachProcess(&CsrProcess->Pcb);
   }

   DPRINT("UserLeaveCo\n");
   UserLeaveCo();

   DPRINT("ZwRequestWaitReplyPort\n");
   Status = ZwRequestWaitReplyPort(WindowsApiPort,
                                   &Request->Header,
                                   &Request->Header);

   DPRINT("UserEnterCo\n");
   UserEnterCo();

   if (CsrProcess != OldProcess)
   {
      DPRINT("KeDetachProcess\n");
      KeDetachProcess();
   }

   if (NT_SUCCESS(Status))
   {
      Status = Request->Status;
   }

   DPRINT("Status %x\n");
   return Status;
}


NTSTATUS
APIENTRY
CsrInsertObject(HANDLE ObjectHandle,
                ACCESS_MASK DesiredAccess,
                PHANDLE Handle)
{
   NTSTATUS Status;
   HANDLE CsrProcessHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID Cid;

   /* Put CSR'S CID */
   Cid.UniqueProcess = CsrProcess->UniqueProcessId;
   Cid.UniqueThread = 0;

   /* Empty Attributes */
   InitializeObjectAttributes(&ObjectAttributes,
                              NULL,
                              0,
                              NULL,
                              NULL);

   /* Get a Handle to Csrss */
   Status = ZwOpenProcess(&CsrProcessHandle,
                          PROCESS_DUP_HANDLE,
                          &ObjectAttributes,
                          &Cid);

   if ((NT_SUCCESS(Status)))
   {
      /* Duplicate the Handle */
      Status = ZwDuplicateObject(NtCurrentProcess(),
                                 ObjectHandle,
                                 CsrProcessHandle,
                                 Handle,
                                 DesiredAccess,
                                 OBJ_INHERIT,
                                 0);

      /* Close our handle to CSRSS */
      ZwClose(CsrProcessHandle);
   }

   return Status;
}

NTSTATUS FASTCALL
CsrCloseHandle(HANDLE Handle)
{
   NTSTATUS Status;
   PEPROCESS OldProcess;

   /* Switch to the process in which the handle is valid */
   OldProcess = PsGetCurrentProcess();
   if (CsrProcess != OldProcess)
   {
      KeAttachProcess(&CsrProcess->Pcb);
   }

   Status = ZwClose(Handle);

   if (CsrProcess != OldProcess)
   {
      KeDetachProcess();
   }

   return Status;
}

/* EOF */
