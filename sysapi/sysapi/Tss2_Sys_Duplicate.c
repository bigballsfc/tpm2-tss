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

TPM_RC Tss2_Sys_Duplicate_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT objectHandle,
    TPMI_DH_OBJECT newParentHandle,
    TPM2B_DATA *encryptionKeyIn,
    TPMT_SYM_DEF_OBJECT *symmetricAlg)
{
    TPM_RC rval;

    if (!sysContext || !symmetricAlg)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(sysContext, TPM_CC_Duplicate);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(objectHandle, SYS_CONTEXT->cmdBuffer,
                                  SYS_CONTEXT->maxCmdSize,
                                  &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(newParentHandle, SYS_CONTEXT->cmdBuffer,
                                  SYS_CONTEXT->maxCmdSize,
                                  &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    if (!encryptionKeyIn) {
        SYS_CONTEXT->decryptNull = 1;

        rval = Tss2_MU_UINT16_Marshal(0, SYS_CONTEXT->cmdBuffer,
                                      SYS_CONTEXT->maxCmdSize,
                                      &SYS_CONTEXT->nextData);
    } else {

        rval = Tss2_MU_TPM2B_DATA_Marshal(encryptionKeyIn, SYS_CONTEXT->cmdBuffer,
                                          SYS_CONTEXT->maxCmdSize,
                                          &SYS_CONTEXT->nextData);
    }

    if (rval)
        return rval;

    rval = Tss2_MU_TPMT_SYM_DEF_OBJECT_Marshal(symmetricAlg,
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

TPM_RC Tss2_Sys_Duplicate_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    TPM2B_DATA *encryptionKeyOut,
    TPM2B_PRIVATE *duplicate,
    TPM2B_ENCRYPTED_SECRET *outSymSeed)
{
    TSS2_RC rval;

    if (!sysContext)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(sysContext);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_DATA_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                        SYS_CONTEXT->maxCmdSize,
                                        &SYS_CONTEXT->nextData,
                                        encryptionKeyOut);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_PRIVATE_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                           SYS_CONTEXT->maxCmdSize,
                                           &SYS_CONTEXT->nextData,
                                           duplicate);
    if (rval)
        return rval;

    return Tss2_MU_TPM2B_ENCRYPTED_SECRET_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                                    SYS_CONTEXT->maxCmdSize,
                                                    &SYS_CONTEXT->nextData,
                                                    outSymSeed);
}

TPM_RC Tss2_Sys_Duplicate(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT objectHandle,
    TPMI_DH_OBJECT newParentHandle,
    TSS2_SYS_CMD_AUTHS const *cmdAuthsArray,
    TPM2B_DATA *encryptionKeyIn,
    TPMT_SYM_DEF_OBJECT *symmetricAlg,
    TPM2B_DATA *encryptionKeyOut,
    TPM2B_PRIVATE *duplicate,
    TPM2B_ENCRYPTED_SECRET *outSymSeed,
    TSS2_SYS_RSP_AUTHS *rspAuthsArray)
{
    TSS2_RC rval;

    if (!symmetricAlg)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = Tss2_Sys_Duplicate_Prepare(sysContext, objectHandle,
                                      newParentHandle, encryptionKeyIn,
                                      symmetricAlg);
    if (rval)
        return rval;

    rval = CommonOneCall(sysContext, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_Duplicate_Complete(sysContext, encryptionKeyOut,
                                       duplicate, outSymSeed);
}
