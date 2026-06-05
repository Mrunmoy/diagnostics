#include "diag/diag.h"

#include <stdio.h>

static enum diag_result example_load(void *user, uint8_t *buffer, size_t buffer_size,
                                     size_t *bytes_read)
{
    (void)user;
    (void)buffer;
    (void)buffer_size;
    *bytes_read = 0u;
    return DIAG_OK;
}

static enum diag_result example_save(void *user, const uint8_t *buffer, size_t size)
{
    (void)user;
    (void)buffer;
    (void)size;
    return DIAG_OK;
}

static enum diag_result example_clear(void *user)
{
    (void)user;
    return DIAG_OK;
}

static enum diag_result example_send(void *user, const uint8_t *buffer, size_t size)
{
    (void)user;
    (void)buffer;
    (void)size;
    return DIAG_OK;
}

static enum diag_result example_receive(void *user, uint8_t *buffer, size_t buffer_size,
                                        size_t *bytes_read)
{
    (void)user;
    (void)buffer;
    (void)buffer_size;
    *bytes_read = 0u;
    return DIAG_OK;
}

int main(void)
{
    struct diag_context_storage   storage = {0};
    struct diag_context          *ctx = NULL;
    const struct diag_config      config = {0};
    const struct diag_storage_ops storage_ops = {
        .load = example_load,
        .save = example_save,
        .clear = example_clear,
    };
    const struct diag_storage diag_storage = {
        .ops = &storage_ops,
        .user = NULL,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 4u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
    };
    const struct diag_transport_ops transport_ops = {
        .send = example_send,
        .receive = example_receive,
    };
    const struct diag_transport transport = {
        .ops = &transport_ops,
        .user = NULL,
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_init failed\n");
        return 1;
    }

    if (diag_storage_attach(ctx, &diag_storage) != DIAG_OK)
    {
        fprintf(stderr, "diag_storage_attach failed\n");
        return 1;
    }

    if (diag_transport_attach(ctx, &transport) != DIAG_OK)
    {
        fprintf(stderr, "diag_transport_attach failed\n");
        return 1;
    }

    if (diag_deinit(ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_deinit failed\n");
        return 1;
    }

    return 0;
}
