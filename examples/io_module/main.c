#include "diag/diag.h"

#include <stdio.h>
#include <string.h>

struct io_bus
{
    uint8_t last_frame[16];
    size_t  last_frame_size;
};

static enum diag_result io_send(void *user, const uint8_t *buffer, size_t size)
{
    struct io_bus *bus = (struct io_bus *)user;

    if (size > sizeof(bus->last_frame))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(bus->last_frame, buffer, size);
    bus->last_frame_size = size;

    return DIAG_OK;
}

static enum diag_result io_receive(void *user, uint8_t *buffer, size_t buffer_size,
                                   size_t *bytes_read)
{
    struct io_bus *bus = (struct io_bus *)user;

    if (buffer_size < bus->last_frame_size)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, bus->last_frame, bus->last_frame_size);
    *bytes_read = bus->last_frame_size;

    return DIAG_OK;
}

int main(void)
{
    static const struct diag_transport_ops transport_ops = {
        .send = io_send,
        .receive = io_receive,
    };

    struct io_bus               bus = {{0}, 0u};
    struct diag_context_storage storage = {0};
    struct diag_context        *ctx = NULL;
    uint8_t                     rx[16] = {0};
    size_t                      rx_size = 0u;
    const uint8_t               request[] = {0x22u, 0x10u, 0x01u};
    const struct diag_config    config = {0};
    const struct diag_identity  identity = {
         .ecosystem_id = 1u,
         .product_id = 40u,
         .device_type = 4u,
         .device_instance = 3u,
         .firmware_stage = 1u,
         .firmware_component = 1u,
         .reserved = 0u,
    };
    const struct diag_transport transport = {
        .ops = &transport_ops,
        .user = &bus,
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK ||
        diag_identity_attach(ctx, &identity) != DIAG_OK ||
        diag_transport_attach(ctx, &transport) != DIAG_OK)
    {
        fprintf(stderr, "io_module: setup failed\n");
        return 1;
    }

    if (transport.ops->send(transport.user, request, sizeof(request)) != DIAG_OK ||
        transport.ops->receive(transport.user, rx, sizeof(rx), &rx_size) != DIAG_OK ||
        rx_size != sizeof(request))
    {
        fprintf(stderr, "io_module: transport loopback failed\n");
        return 1;
    }

    printf("io_module: looped back %lu diagnostic bytes\n", (unsigned long)rx_size);

    return diag_deinit(ctx) == DIAG_OK ? 0 : 1;
}
