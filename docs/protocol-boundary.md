# Protocol Boundary

This library is not a CAN protocol stack and not a UDS stack.

It should be possible to build a UDS-like server on top of the core, but the
core API should remain usable by applications that do not use UDS framing,
service IDs, negative response codes, sessions, or security access.

## Suggested Command Mapping

A project-specific protocol can map incoming commands to core APIs:

| Command | Core API |
| --- | --- |
| Create code | `diag_dtc_register` |
| Set code active | `diag_dtc_set_active` |
| Set code inactive | `diag_dtc_set_inactive` |
| Read code | `diag_dtc_get` |
| List codes | `diag_dtc_list` |
| Clear code | `diag_dtc_clear` |
| Clear all | `diag_dtc_clear_all` |
| Read counter | `diag_dtc_get` |
| Reset counter | `diag_dtc_reset_counter` |
| Save state | `diag_save` |
| Load state | `diag_load` |

