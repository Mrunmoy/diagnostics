#ifndef DIAG_CAPSULE_H
#define DIAG_CAPSULE_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

#define DIAG_CAPSULE_SCHEMA_VERSION 1u
#define DIAG_CAPSULE_MAX_SECTIONS 8u
#define DIAG_CAPSULE_HEADER_SIZE 24u
#define DIAG_CAPSULE_SECTION_ENTRY_SIZE 16u
#define DIAG_CAPSULE_MAGIC 0x50434744u

enum diag_capsule_section_type
{
    DIAG_CAPSULE_SECTION_BOOTLOADER_DTC = 1,
    DIAG_CAPSULE_SECTION_APPLICATION_DTC = 2,
    DIAG_CAPSULE_SECTION_LIFECYCLE = 3,
    DIAG_CAPSULE_SECTION_HANDOFF = 4,
    DIAG_CAPSULE_SECTION_RESET_COUNTERS = 5,
    DIAG_CAPSULE_SECTION_RESERVED = 0xFFFFu
};

enum diag_capsule_section_owner
{
    DIAG_CAPSULE_SECTION_OWNER_UNKNOWN = 0,
    DIAG_CAPSULE_SECTION_OWNER_BOOTLOADER = 1,
    DIAG_CAPSULE_SECTION_OWNER_APPLICATION = 2,
    DIAG_CAPSULE_SECTION_OWNER_SHARED = 3,
    DIAG_CAPSULE_SECTION_OWNER_RESERVED = 4
};

struct diag_capsule_section
{
    uint16_t type;
    uint16_t version;
    uint32_t offset;
    uint32_t length;
    uint32_t used_length;
};

struct diag_capsule_descriptor
{
    uint16_t schema_version;
    uint16_t section_count;
    uint32_t total_length;
    uint32_t generation;
    uint32_t content_crc32;
    struct diag_capsule_section sections[DIAG_CAPSULE_MAX_SECTIONS];
};

uint32_t diag_capsule_crc32(const uint8_t *data, size_t length);

enum diag_result diag_capsule_encode_v1(uint8_t *buffer, size_t capacity,
                                        const struct diag_capsule_descriptor *descriptor,
                                        size_t *encoded_length);

enum diag_result diag_capsule_decode(const uint8_t *buffer, size_t length,
                                     struct diag_capsule_descriptor *out_descriptor);

enum diag_result diag_capsule_section_owner_from_type(uint16_t type,
                                                      enum diag_capsule_section_owner *out_owner);

enum diag_result diag_capsule_find_section_by_type(const struct diag_capsule_descriptor *descriptor,
                                                   uint16_t type,
                                                   const struct diag_capsule_section **out_section);

enum diag_result
diag_capsule_find_section_by_owner(const struct diag_capsule_descriptor *descriptor,
                                   enum diag_capsule_section_owner owner,
                                   const struct diag_capsule_section **out_section);

enum diag_result
diag_capsule_validate_section_bounds(const struct diag_capsule_descriptor *descriptor,
                                     const struct diag_capsule_section *section);

enum diag_result diag_capsule_copy_section_payload(const uint8_t *capsule, size_t capsule_length,
                                                   const struct diag_capsule_descriptor *descriptor,
                                                   const struct diag_capsule_section *section,
                                                   uint8_t *out_payload, size_t out_capacity,
                                                   size_t *out_length);

enum diag_result diag_capsule_copy_section_payload_by_type(
    const uint8_t *capsule, size_t capsule_length, const struct diag_capsule_descriptor *descriptor,
    uint16_t type, uint8_t *out_payload, size_t out_capacity, size_t *out_length);

#endif
