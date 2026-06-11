#include "diag/diag.h"

#include <stdio.h>
#include <string.h>

struct capsule_bank
{
    uint8_t bytes[192];
    size_t  used;
};

static enum diag_result bank_load(void *user, uint8_t *buffer, size_t buffer_size,
                                  size_t *bytes_read)
{
    struct capsule_bank *bank = (struct capsule_bank *)user;

    if (buffer_size < bank->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, bank->bytes, bank->used);
    *bytes_read = bank->used;

    return DIAG_OK;
}

static enum diag_result bank_save(void *user, const uint8_t *buffer, size_t size)
{
    struct capsule_bank *bank = (struct capsule_bank *)user;

    if (size > sizeof(bank->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(bank->bytes, buffer, size);
    bank->used = size;

    return DIAG_OK;
}

static enum diag_result bank_clear(void *user)
{
    struct capsule_bank *bank = (struct capsule_bank *)user;

    bank->used = 0u;

    return DIAG_OK;
}

static struct diag_storage make_bank_storage(struct capsule_bank *bank, uint8_t *buffer,
                                             size_t buffer_size)
{
    static const struct diag_storage_ops ops = {
        .load = bank_load,
        .save = bank_save,
        .clear = bank_clear,
    };
    const struct diag_storage storage = {
        .ops = &ops,
        .user = bank,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 4u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
        .capsule_buffer = buffer,
        .capsule_buffer_size = buffer_size,
    };

    return storage;
}

static enum diag_result save_bootloader_bank(struct capsule_bank *bank)
{
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    uint8_t                      capsule_buffer[192] = {0};
    const struct diag_config     config = {0};
    struct diag_lifecycle_config lifecycle_config = {0};
    const struct diag_storage    bank_storage =
        make_bank_storage(bank, capsule_buffer, sizeof(capsule_buffer));

    lifecycle_config.reset_counter_policy = DIAG_RESET_COUNTER_POLICY_EVERY_N;
    lifecycle_config.reset_count_interval = 1u;

    if (diag_init(&storage, &config, &ctx) != DIAG_OK ||
        diag_storage_attach(ctx, &bank_storage) != DIAG_OK ||
        diag_lifecycle_attach(ctx, &lifecycle_config) != DIAG_OK ||
        diag_lifecycle_observe_reset(ctx, DIAG_RESET_REASON_UPDATE) != DIAG_OK ||
        diag_save(ctx) != DIAG_OK)
    {
        return DIAG_ERROR_STORAGE;
    }

    return diag_deinit(ctx);
}

static enum diag_result save_application_bank(struct capsule_bank *bank)
{
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    struct diag_dtc_snapshot     dtc_records[2] = {0};
    uint8_t                      capsule_buffer[192] = {0};
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = dtc_records,
        .capacity = 2u,
    };
    const struct diag_storage bank_storage =
        make_bank_storage(bank, capsule_buffer, sizeof(capsule_buffer));

    if (diag_init(&storage, &config, &ctx) != DIAG_OK ||
        diag_storage_attach(ctx, &bank_storage) != DIAG_OK ||
        diag_dtc_attach(ctx, &dtc_config) != DIAG_OK ||
        diag_dtc_register_fault(ctx, 9u, 0x060001u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK ||
        diag_dtc_set_fault_test_failed(ctx, 9u) != DIAG_OK ||
        diag_dtc_operation_cycle(ctx) != DIAG_OK || diag_save(ctx) != DIAG_OK)
    {
        return DIAG_ERROR_STORAGE;
    }

    return diag_deinit(ctx);
}

int main(void)
{
    struct capsule_bank bootloader_bank = {{0}, 0u};
    struct capsule_bank application_bank = {{0}, 0u};

    if (save_bootloader_bank(&bootloader_bank) != DIAG_OK ||
        save_application_bank(&application_bank) != DIAG_OK)
    {
        fprintf(stderr, "bootloader_app_shared: save failed\n");
        return 1;
    }

    if (bootloader_bank.used == 0u || application_bank.used == 0u)
    {
        fprintf(stderr, "bootloader_app_shared: expected two independent banks\n");
        return 1;
    }

    printf("bootloader_app_shared: bootloader bank %lu bytes, application bank %lu bytes\n",
           (unsigned long)bootloader_bank.used, (unsigned long)application_bank.used);

    return 0;
}
