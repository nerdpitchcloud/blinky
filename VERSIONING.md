# Blinky Versioning Guide

Blinky follows [Semantic Versioning](https://semver.org/) (SemVer) with automated patch releases.

## Version Format

Versions follow the format: `vMAJOR.MINOR.PATCH`

- **MAJOR**: Breaking changes (manual only)
- **MINOR**: New features, backward compatible (manual only)
- **PATCH**: Bug fixes, backward compatible (automatic or manual)

## Automatic Versioning

### Patch Releases (Automatic)

Every push to the `main` branch automatically:
1. Increments the patch version
2. Creates a new tag
3. Builds binaries
4. Creates a GitHub release

**Example:**
- Current version: `v1.2.3`
- Push to main → Automatic release: `v1.2.4`

### Manual Releases (Major/Minor)

Major and minor version bumps must be done manually using the release script.

## Creating Manual Releases

### Using the Release Script (Recommended)

```bash
# Increment major version (resets minor and patch to 0)
./scripts/release.sh major
# Example: v1.2.3 -> v2.0.0

# Increment minor version (resets patch to 0)
./scripts/release.sh minor
# Example: v1.2.3 -> v1.3.0

# Increment patch version (if needed manually)
./scripts/release.sh patch
# Example: v1.2.3 -> v1.2.4
```

### Manual Tag Creation

If you prefer to create tags manually:

```bash
# Get current version
git describe --tags --abbrev=0

# Create new tag (following SemVer rules)
git tag -a v2.0.0 -m "Release v2.0.0"

# Push tag
git push origin v2.0.0
```

## Version Rules

The CI/CD pipeline enforces these rules:

### Major Version Bump
When incrementing the major version, minor and patch **must be 0**.

✅ Valid:
- `v1.5.3` → `v2.0.0`

❌ Invalid:
- `v1.5.3` → `v2.0.1` (patch must be 0)
- `v1.5.3` → `v2.1.0` (minor must be 0)

### Minor Version Bump
When incrementing the minor version, patch **must be 0**.

✅ Valid:
- `v1.5.3` → `v1.6.0`

❌ Invalid:
- `v1.5.3` → `v1.6.1` (patch must be 0)

### Patch Version Bump
Patch can be any increment.

✅ Valid:
- `v1.5.3` → `v1.5.4`
- `v1.5.3` → `v1.5.10`

## When to Bump Versions

### Major Version (X.0.0)
Increment when making **breaking changes**:
- Removing features
- Changing API/protocol in incompatible ways
- Changing default behavior that breaks existing setups
- Removing or renaming command-line options

**Example:** Changing the protocol format, requiring agents to be updated

### Minor Version (x.Y.0)
Increment when adding **new features** (backward compatible):
- Adding new monitoring capabilities
- Adding new command-line options
- Adding new API endpoints
- Improving existing features without breaking changes

**Example:** Adding support for monitoring a new service type

### Patch Version (x.y.Z)
Increment for **bug fixes and minor improvements**:
- Fixing bugs
- Performance improvements
- Documentation updates
- Security patches
- Dependency updates

**Example:** Fixing a memory leak, improving error messages

## Workflow Examples

### Scenario 1: Regular Development
```bash
# Developer pushes bug fix to main
git push origin main

# CI automatically creates v1.2.4
# No manual intervention needed
```

### Scenario 2: New Feature Release
```bash
# Developer completes new feature
git push origin main

# Maintainer creates minor release
./scripts/release.sh minor

# CI creates v1.3.0 with the new feature
```

### Scenario 3: Breaking Change
```bash
# Developer completes breaking change
git push origin main

# Maintainer creates major release
./scripts/release.sh major

# CI creates v2.0.0
```

## Version Validation

The CI/CD pipeline includes a version validation workflow that:
- Checks version format (must be vX.Y.Z)
- Validates version increment rules
- Ensures major bumps reset minor and patch to 0
- Ensures minor bumps reset patch to 0
- Prevents invalid version sequences

If validation fails, the release will not be created.

## Checking Current Version

```bash
# Get latest tag
git describe --tags --abbrev=0

# Get version from binary
./build/agent/blinky-agent --version
./build/collector/blinky-collector --version
```

## Release Artifacts

Each release includes:
- Pre-built binaries for Linux AMD64
- Changelog with commit history
- Source code archive
- Release notes

## Best Practices

1. **Let patch versions auto-increment** - Don't manually create patch releases unless necessary
2. **Plan major/minor releases** - Coordinate breaking changes and new features
3. **Update CHANGELOG** - Document significant changes before major/minor releases
4. **Test before tagging** - Ensure main branch is stable before creating manual releases
5. **Use the release script** - It validates version rules automatically
6. **Communicate breaking changes** - Announce major version bumps to users

## Troubleshooting

### Tag Already Exists
```bash
# Delete local tag
git tag -d v1.2.3

# Delete remote tag
git push origin :refs/tags/v1.2.3

# Create new tag
./scripts/release.sh minor
```

### Wrong Version Created
```bash
# Delete the incorrect tag
git push origin :refs/tags/v1.2.3

# Create correct tag
./scripts/release.sh major
```

### CI Validation Failed
Check the error message in GitHub Actions. Common issues:
- Version doesn't follow X.Y.Z format
- Major bump didn't reset minor/patch to 0
- Minor bump didn't reset patch to 0
- Version is not an increment of previous version

## References

- [Semantic Versioning Specification](https://semver.org/)
- [GitHub Releases](https://github.com/nerdpitchcloud/blinky/releases)
- [GitHub Actions Workflows](https://github.com/nerdpitchcloud/blinky/actions)
