# Security Policy

## Supported Versions

We release patches for security vulnerabilities for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| 0.1.x   | :white_check_mark: |
| < 0.1   | :x:                |

## Reporting a Vulnerability

We take the security of Blinky seriously. If you believe you have found a security vulnerability, please report it to us as described below.

### Where to Report

**Please do not report security vulnerabilities through public GitHub issues.**

Instead, please report them via one of the following methods:

1. **GitHub Security Advisories** (Preferred)
   - Go to the [Security tab](https://github.com/nerdpitchcloud/blinky/security/advisories)
   - Click "Report a vulnerability"
   - Fill out the form with details

2. **Email**
   - Send an email to: security@nerdpitchcloud.com
   - Include "Blinky Security" in the subject line

### What to Include

Please include the following information in your report:

- Type of vulnerability (e.g., buffer overflow, SQL injection, cross-site scripting, etc.)
- Full paths of source file(s) related to the vulnerability
- Location of the affected source code (tag/branch/commit or direct URL)
- Step-by-step instructions to reproduce the issue
- Proof-of-concept or exploit code (if possible)
- Impact of the issue, including how an attacker might exploit it

### Response Timeline

- **Initial Response**: Within 48 hours of report submission
- **Status Update**: Within 7 days with assessment and expected timeline
- **Fix Release**: Depends on severity and complexity
  - Critical: Within 7 days
  - High: Within 14 days
  - Medium: Within 30 days
  - Low: Next regular release

### Disclosure Policy

- Security issues are disclosed publicly after a fix is released
- We will credit reporters in the security advisory (unless anonymity is requested)
- We follow coordinated disclosure practices

## Security Best Practices

When deploying Blinky, we recommend the following security practices:

### Network Security

1. **Firewall Configuration**
   - Restrict access to port 9092 (agent API) to trusted networks only
   - Restrict access to port 9090 (collector WebSocket) to monitored hosts only
   - Use firewall rules to limit connections

2. **TLS/SSL**
   - Use a reverse proxy (nginx, Apache) with TLS for production deployments
   - Never expose the agent API directly to the internet without encryption

### System Security

1. **File Permissions**
   - Configuration files: `chmod 640 /etc/blinky/config.toml`
   - Binary: `chmod 755 /usr/local/bin/blinky-agent`
   - Storage directory: `chmod 750 /var/lib/blinky`

2. **User Privileges**
   - Run the agent with minimal required privileges
   - Consider using a dedicated service account
   - Avoid running as root when possible (note: some metrics require root)

3. **Configuration**
   - Review and customize `/etc/blinky/config.toml`
   - Disable monitors you don't need
   - Limit API bind address to localhost if not needed externally

### Monitoring Security

1. **Access Control**
   - Implement authentication on the collector dashboard
   - Use network segmentation for monitoring infrastructure
   - Regularly review access logs

2. **Data Protection**
   - Metrics may contain sensitive system information
   - Ensure proper access controls on stored metrics
   - Consider data retention policies

3. **Updates**
   - Keep Blinky updated to the latest version
   - Subscribe to security advisories
   - Use `blinky-agent upgrade` regularly

### Docker/Container Security

1. **Container Isolation**
   - Run collector in isolated network
   - Use read-only root filesystem where possible
   - Limit container capabilities

2. **Image Security**
   - Use official images only
   - Scan images for vulnerabilities
   - Keep base images updated

## Known Security Considerations

### Agent Privileges

The Blinky agent requires elevated privileges for certain monitoring features:

- **SMART monitoring**: Requires root access to read disk health data
- **Container monitoring**: Requires access to Docker/Podman sockets
- **Systemd monitoring**: Requires access to systemd D-Bus

If these features are not needed, disable them in the configuration to reduce the attack surface.

### Metrics Data Sensitivity

Blinky collects system metrics that may include:

- Hostnames and IP addresses
- Running processes and services
- Container names and configurations
- Kubernetes cluster information
- Disk usage and mount points

Ensure this data is protected according to your organization's security policies.

### Local Storage

Metrics are stored locally in `/var/lib/blinky/metrics` as JSON files:

- Files are readable by the user running the agent
- No encryption at rest by default
- Automatic rotation limits data retention
- Consider encrypting the storage directory if needed

### HTTP API

The agent HTTP API (port 9092) exposes metrics without authentication by default:

- Bind to localhost only if external access is not needed
- Use a reverse proxy with authentication for production
- Consider implementing IP whitelisting
- Monitor API access logs

## Security Updates

Security updates are released as patch versions (e.g., 0.1.x) and include:

- Security fixes in release notes
- CVE identifiers when applicable
- Upgrade instructions
- Affected versions

Subscribe to releases on GitHub to receive notifications of security updates.

## Vulnerability Disclosure

Past security advisories can be found at:
https://github.com/nerdpitchcloud/blinky/security/advisories

## Contact

For security-related questions or concerns:
- Email: security@nerdpitchcloud.com
- GitHub: https://github.com/nerdpitchcloud/blinky/security

## Acknowledgments

We appreciate the security research community's efforts in responsibly disclosing vulnerabilities. Contributors who report valid security issues will be acknowledged in our security advisories (unless they prefer to remain anonymous).
