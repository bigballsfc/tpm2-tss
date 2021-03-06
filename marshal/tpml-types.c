/***********************************************************************
 * Copyright (c) 2015 - 2017, Intel Corporation
 *
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

#include <inttypes.h>
#include <string.h>

#include "sapi/tss2_mu.h"
#include "sapi/tpm20.h"
#include "tss2_endian.h"
#include "log.h"

#define ADDR &
#define VAL
#define TAB_SIZE(tab) (sizeof(tab) / sizeof(tab[0]))

#define TPML_MARSHAL(type, marshal_func, buf_name, op) \
TSS2_RC Tss2_MU_##type##_Marshal(type const *src, uint8_t buffer[], \
                                 size_t buffer_size, size_t *offset) \
{ \
    size_t  local_offset = 0; \
    UINT32 i, count = 0; \
    TSS2_RC ret = TSS2_RC_SUCCESS; \
    uint8_t *buf_ptr = buffer; \
    uint8_t local_buffer[buffer_size]; \
\
    if (offset != NULL) { \
        LOG (INFO, "offset non-NULL, initial value: %zu", *offset); \
        local_offset = *offset; \
    } \
\
    if (src == NULL) { \
        LOG (WARNING, "src is NULL"); \
        return TSS2_TYPES_RC_BAD_REFERENCE; \
    } \
\
    if (buffer == NULL && offset == NULL) { \
        LOG (WARNING, "buffer and offset parameter are NULL"); \
        return TSS2_TYPES_RC_BAD_REFERENCE; \
    } else if (buffer_size < local_offset || \
               buffer_size - local_offset < sizeof(count)) { \
        LOG (WARNING, \
             "buffer_size: %zu with offset: %zu are insufficient for object " \
             "of size %zu", \
             buffer_size, \
             local_offset, \
             sizeof(count)); \
        return TSS2_TYPES_RC_INSUFFICIENT_BUFFER; \
    } \
\
    if (src->count > TAB_SIZE(src->buf_name)) { \
        LOG (WARNING, "count too big"); \
        return TSS2_SYS_RC_BAD_VALUE; \
    } \
\
    if (buf_ptr == NULL) \
        buf_ptr = local_buffer; \
\
    LOG (DEBUG, \
         "Marshalling " #type " from 0x%" PRIxPTR " to buffer 0x%" PRIxPTR \
         " at index 0x%zx", \
         (uintptr_t)&src, \
         (uintptr_t)buf_ptr, \
         local_offset); \
\
    ret = Tss2_MU_UINT32_Marshal(src->count, buf_ptr, buffer_size, &local_offset); \
    if (ret) \
        return ret; \
\
    for (i = 0; i < src->count; i++) \
    { \
        ret = marshal_func(op src->buf_name[i], buf_ptr, buffer_size, &local_offset); \
        if (ret) \
            return ret; \
    } \
    if (offset != NULL) { \
        *offset = local_offset; \
        LOG (DEBUG, "offset parameter non-NULL updated to %zu", *offset); \
    } \
\
    return TSS2_RC_SUCCESS; \
}

#define TPML_UNMARSHAL(type, unmarshal_func, buf_name) \
TSS2_RC Tss2_MU_##type##_Unmarshal(uint8_t const buffer[], size_t buffer_size, \
                                   size_t *offset, type *dest) \
{ \
    size_t  local_offset = 0; \
    UINT32 i, count = 0; \
    TSS2_RC ret = TSS2_RC_SUCCESS; \
    type local_dst; \
\
    if (offset != NULL) { \
        LOG (INFO, "offset non-NULL, initial value: %zu", *offset); \
        local_offset = *offset; \
    } \
\
    if (buffer == NULL || (dest == NULL && offset == NULL)) { \
        LOG (WARNING, "buffer or dest and offset parameter are NULL"); \
        return TSS2_TYPES_RC_BAD_REFERENCE; \
    } else if (buffer_size < local_offset || \
               sizeof(count) > buffer_size - local_offset) \
    { \
        LOG (WARNING, \
             "buffer_size: %zu with offset: %zu are insufficient for object " \
             "of size %zu", \
             buffer_size, \
             local_offset, \
             sizeof(count)); \
        return TSS2_TYPES_RC_INSUFFICIENT_BUFFER; \
    } \
\
    LOG (DEBUG, \
         "Unmarshalling " #type " from 0x%" PRIxPTR " to buffer 0x%" PRIxPTR \
         " at index 0x%zx", \
         (uintptr_t)buffer, \
         (uintptr_t)dest, \
         local_offset); \
\
    if (dest == NULL) \
        dest = &local_dst; \
\
    ret = Tss2_MU_UINT32_Unmarshal(buffer, buffer_size, &local_offset, &count); \
    if (ret) \
        return ret; \
\
    if (count > TAB_SIZE(dest->buf_name)) { \
        LOG (WARNING, "count too big"); \
        return TSS2_SYS_RC_MALFORMED_RESPONSE; \
    } \
\
    dest->count = count; \
\
    for (i = 0; i < count; i++) \
    { \
        ret = unmarshal_func(buffer, buffer_size, &local_offset, &dest->buf_name[i]); \
        if (ret) \
            return ret; \
    } \
\
    if (offset != NULL) { \
        *offset = local_offset; \
        LOG (DEBUG, "offset parameter non-NULL, updated to %zu", *offset); \
    } \
\
    return TSS2_RC_SUCCESS; \
}

/*
 * These macros expand to (un)marshal functions for each of the TPML types
 * the specification part 2.
 */
TPML_MARSHAL(TPML_CC, Tss2_MU_TPM_CC_Marshal, commandCodes, VAL)
TPML_UNMARSHAL(TPML_CC, Tss2_MU_TPM_CC_Unmarshal, commandCodes)
TPML_MARSHAL(TPML_CCA, Tss2_MU_TPMA_CC_Marshal, commandAttributes, VAL)
TPML_UNMARSHAL(TPML_CCA, Tss2_MU_TPMA_CC_Unmarshal, commandAttributes)
TPML_MARSHAL(TPML_ALG, Tss2_MU_UINT16_Marshal, algorithms, VAL)
TPML_UNMARSHAL(TPML_ALG, Tss2_MU_UINT16_Unmarshal, algorithms)
TPML_MARSHAL(TPML_HANDLE, Tss2_MU_UINT32_Marshal, handle, VAL)
TPML_UNMARSHAL(TPML_HANDLE, Tss2_MU_UINT32_Unmarshal, handle)
TPML_MARSHAL(TPML_DIGEST, Tss2_MU_TPM2B_DIGEST_Marshal, digests, ADDR)
TPML_UNMARSHAL(TPML_DIGEST, Tss2_MU_TPM2B_DIGEST_Unmarshal, digests)
TPML_MARSHAL(TPML_ALG_PROPERTY, Tss2_MU_TPMS_ALG_PROPERTY_Marshal, algProperties, ADDR)
TPML_UNMARSHAL(TPML_ALG_PROPERTY, Tss2_MU_TPMS_ALG_PROPERTY_Unmarshal, algProperties)
TPML_MARSHAL(TPML_ECC_CURVE, Tss2_MU_UINT16_Marshal, eccCurves, VAL)
TPML_UNMARSHAL(TPML_ECC_CURVE, Tss2_MU_UINT16_Unmarshal, eccCurves)
TPML_MARSHAL(TPML_TAGGED_TPM_PROPERTY, Tss2_MU_TPMS_TAGGED_PROPERTY_Marshal, tpmProperty, ADDR)
TPML_UNMARSHAL(TPML_TAGGED_TPM_PROPERTY, Tss2_MU_TPMS_TAGGED_PROPERTY_Unmarshal, tpmProperty)
TPML_MARSHAL(TPML_TAGGED_PCR_PROPERTY, Tss2_MU_TPMS_TAGGED_PCR_SELECT_Marshal, pcrProperty, ADDR)
TPML_UNMARSHAL(TPML_TAGGED_PCR_PROPERTY, Tss2_MU_TPMS_TAGGED_PCR_SELECT_Unmarshal, pcrProperty)
TPML_MARSHAL(TPML_PCR_SELECTION, Tss2_MU_TPMS_PCR_SELECTION_Marshal, pcrSelections, ADDR)
TPML_UNMARSHAL(TPML_PCR_SELECTION, Tss2_MU_TPMS_PCR_SELECTION_Unmarshal, pcrSelections)
TPML_MARSHAL(TPML_DIGEST_VALUES, Tss2_MU_TPMT_HA_Marshal, digests, ADDR)
TPML_UNMARSHAL(TPML_DIGEST_VALUES, Tss2_MU_TPMT_HA_Unmarshal, digests)
TPML_MARSHAL(TPML_INTEL_PTT_PROPERTY, Tss2_MU_UINT32_Marshal, property, VAL)
TPML_UNMARSHAL(TPML_INTEL_PTT_PROPERTY, Tss2_MU_UINT32_Unmarshal, property)
