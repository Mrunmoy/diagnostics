#ifndef MODBUS_RTU_TRANSPORT_MODBUS_RTU_DIAG_LINK_HPP
#define MODBUS_RTU_TRANSPORT_MODBUS_RTU_DIAG_LINK_HPP

#include "example_diag_tool.h"
#include "serial_diag_link.hpp"

#include <chrono>
#include <cstdint>

namespace diag_modbus_rtu
{

constexpr std::uint8_t kDiagnosticFunction = 0x41u;
constexpr std::uint8_t kDefaultSlaveAddress = 0x11u;

bool receiveRequest(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                    struct example_diag_frame &outFrame, std::chrono::milliseconds timeout);
bool receiveResponse(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                     struct example_diag_frame &outFrame, std::chrono::milliseconds timeout);

void sendRequest(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                 const struct example_diag_frame &frame);
void sendResponse(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                  const struct example_diag_frame &frame);

} // namespace diag_modbus_rtu

#endif
