/***********************************************************************;
 * Copyright (c) 2015 - 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 ***********************************************************************/

#include "sapi/tpm20.h"
#include "sysapi_util.h"

TPM_RC Tss2_Sys_Create_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT parentHandle,
    TPM2B_SENSITIVE_CREATE *inSensitive,
    TPM2B_PUBLIC *inPublic,
    TPM2B_DATA *outsideInfo,
    TPML_PCR_SELECTION *creationPCR)
{
    TPM_RC rval;

    if (!sysContext || !creationPCR)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(sysContext, TPM_CC_Create);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(parentHandle, SYS_CONTEXT->cmdBuffer,
                                  SYS_CONTEXT->maxCmdSize,
                                  &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    if (!inSensitive) {
        SYS_CONTEXT->decryptNull = 1;

        rval = Tss2_MU_UINT16_Marshal(0, SYS_CONTEXT->cmdBuffer,
                                      SYS_CONTEXT->maxCmdSize,
                                      &SYS_CONTEXT->nextData);
    } else {

        rval = Tss2_MU_TPM2B_SENSITIVE_CREATE_Marshal(inSensitive,
                                                      SYS_CONTEXT->cmdBuffer,
                                                      SYS_CONTEXT->maxCmdSize,
                                                      &SYS_CONTEXT->nextData);
    }

    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_PUBLIC_Marshal(inPublic, SYS_CONTEXT->cmdBuffer,
                                        SYS_CONTEXT->maxCmdSize,
                                        &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_DATA_Marshal(outsideInfo, SYS_CONTEXT->cmdBuffer,
                                      SYS_CONTEXT->maxCmdSize,
                                      &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_TPML_PCR_SELECTION_Marshal(creationPCR,
                                              SYS_CONTEXT->cmdBuffer,
                                              SYS_CONTEXT->maxCmdSize,
                                              &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    SYS_CONTEXT->decryptAllowed = 1;
    SYS_CONTEXT->encryptAllowed = 1;
    SYS_CONTEXT->authAllowed = 1;

    return CommonPrepareEpilogue(sysContext);
}

TPM_RC Tss2_Sys_Create_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    TPM2B_PRIVATE *outPrivate,
    TPM2B_PUBLIC *outPublic,
    TPM2B_CREATION_DATA *creationData,
    TPM2B_DIGEST *creationHash,
    TPMT_TK_CREATION *creationTicket)
{
    TPM_RC rval;

    if (!sysContext)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(sysContext);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_PRIVATE_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                           SYS_CONTEXT->maxCmdSize,
                                           &SYS_CONTEXT->nextData,
                                           outPrivate);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_PUBLIC_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                          SYS_CONTEXT->maxCmdSize,
                                          &SYS_CONTEXT->nextData,
                                          outPublic);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_CREATION_DATA_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                                 SYS_CONTEXT->maxCmdSize,
                                                 &SYS_CONTEXT->nextData,
                                                 creationData);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_DIGEST_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                          SYS_CONTEXT->maxCmdSize,
                                          &SYS_CONTEXT->nextData,
                                          creationHash);
    if (rval)
        return rval;

    return Tss2_MU_TPMT_TK_CREATION_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                          SYS_CONTEXT->maxCmdSize,
                                          &SYS_CONTEXT->nextData,
                                          creationTicket);
}

TPM_RC Tss2_Sys_Create(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT parentHandle,
    TSS2_SYS_CMD_AUTHS const *cmdAuthsArray,
    TPM2B_SENSITIVE_CREATE *inSensitive,
    TPM2B_PUBLIC *inPublic,
    TPM2B_DATA *outsideInfo,
    TPML_PCR_SELECTION *creationPCR,
    TPM2B_PRIVATE *outPrivate,
    TPM2B_PUBLIC *outPublic,
    TPM2B_CREATION_DATA *creationData,
    TPM2B_DIGEST *creationHash,
    TPMT_TK_CREATION *creationTicket,
    TSS2_SYS_RSP_AUTHS *rspAuthsArray)
{
    TSS2_RC rval;

    if (!creationPCR)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = Tss2_Sys_Create_Prepare(sysContext, parentHandle, inSensitive,
                                   inPublic, outsideInfo, creationPCR);
    if (rval)
        return rval;

    rval = CommonOneCall(sysContext, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_Create_Complete(sysContext, outPrivate, outPublic,
                                    creationData, creationHash, creationTicket);
}
