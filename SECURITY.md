# Security Policy

## Supported scope

The open source CoreLib scope is:

- stable ABI and language bindings
- provider integration for OpenSSL and liboqs
- fail-closed error handling
- watchdog and inference request validation

The library intentionally does not ship custom cryptographic algorithms.

## Reporting vulnerabilities

Do not open public issues for suspected vulnerabilities in unreleased production deployments. Send a private report to the HESIA maintainers, including:

- affected commit or release
- platform and compiler
- provider versions
- reproduction steps
- expected and observed behavior

## Production guidance

- Build cryptography with audited provider versions.
- Disable demo or soft-sign workflows in production.
- Treat `HESIA_STATUS_UNSUPPORTED` as a deployment failure if a feature is mandatory.
- Wipe secret buffers with `hesia_secure_zero`.
- Record target-side proof before claiming support for a device profile.
