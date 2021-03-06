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

TPM_RC Tss2_Sys_PCR_Read_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPML_PCR_SELECTION *pcrSelectionIn)
{
    TSS2_RC rval;

    if (!sysContext || !pcrSelectionIn)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(sysContext, TPM_CC_PCR_Read);
    if (rval)
        return rval;

    rval = Tss2_MU_TPML_PCR_SELECTION_Marshal(pcrSelectionIn,
                                              SYS_CONTEXT->cmdBuffer,
                                              SYS_CONTEXT->maxCmdSize,
                                              &SYS_CONTEXT->nextData);
    if (rval)
        return rval;

    SYS_CONTEXT->decryptAllowed = 0;
    SYS_CONTEXT->encryptAllowed = 0;
    SYS_CONTEXT->authAllowed = 1;

    return CommonPrepareEpilogue(sysContext);
}

TPM_RC Tss2_Sys_PCR_Read_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    UINT32 *pcrUpdateCounter,
    TPML_PCR_SELECTION *pcrSelectionOut,
    TPML_DIGEST *pcrValues)
{
    TSS2_RC rval;

    if (!sysContext)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(sysContext);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                    SYS_CONTEXT->maxCmdSize,
                                    &SYS_CONTEXT->nextData,
                                    pcrUpdateCounter);
    if (rval)
        return rval;

    rval = Tss2_MU_TPML_PCR_SELECTION_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                                SYS_CONTEXT->maxCmdSize,
                                                &SYS_CONTEXT->nextData,
                                                pcrSelectionOut);
    if (rval)
        return rval;

    return Tss2_MU_TPML_DIGEST_Unmarshal(SYS_CONTEXT->cmdBuffer,
                                         SYS_CONTEXT->maxCmdSize,
                                         &SYS_CONTEXT->nextData, pcrValues);
}

TPM_RC Tss2_Sys_PCR_Read(
    TSS2_SYS_CONTEXT *sysContext,
    TSS2_SYS_CMD_AUTHS const *cmdAuthsArray,
    TPML_PCR_SELECTION *pcrSelectionIn,
    UINT32 *pcrUpdateCounter,
    TPML_PCR_SELECTION *pcrSelectionOut,
    TPML_DIGEST *pcrValues,
    TSS2_SYS_RSP_AUTHS *rspAuthsArray)
{
    TSS2_RC rval;

    if (!pcrSelectionIn)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = Tss2_Sys_PCR_Read_Prepare(sysContext, pcrSelectionIn);
    if (rval)
        return rval;

    rval = CommonOneCall(sysContext, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_PCR_Read_Complete(sysContext, pcrUpdateCounter,
                                      pcrSelectionOut, pcrValues);
}
