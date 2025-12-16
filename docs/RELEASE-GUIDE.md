# Release Guide for Maintainers

This guide explains how to create releases for Blinky.

## Automatic Releases (Patch Versions)

**No action needed!** Every push to `main` automatically:
- Increments patch version (e.g., v1.2.3 → v1.2.4)
- Creates a tag
- Builds binaries
- Publishes release

## Manual Releases (Major/Minor Versions)

For breaking changes or new features, use the GitHub Actions UI.

### Step-by-Step Instructions

#### 1. Navigate to Actions Tab

Go to: https://github.com/nerdpitchcloud/blinky/actions

#### 2. Select Manual Release Workflow

- Click on "Manual Release" in the left sidebar
- Click the "Run workflow" button (top right)

#### 3. Configure Release

A dropdown will appear with options:

**Branch**: Leave as `main` (default)

**Version bump type**: Choose one:
- `major` - For breaking changes
  - Example: v1.5.3 → v2.0.0
  - Use when: API changes, removing features, breaking compatibility
  
- `minor` - For new features  
  - Example: v1.5.3 → v1.6.0
  - Use when: Adding features, improvements, backward compatible changes

#### 4. Run Workflow

Click "Run workflow" button to start the release process.

#### 5. Monitor Progress

- The workflow will appear in the runs list
- Click on it to see progress
- Takes ~2-3 minutes to complete

#### 6. Verify Release

Once complete:
- Check the [Releases page](https://github.com/nerdpitchcloud/blinky/releases)
- New release should be published with binaries
- Changelog is auto-generated from commits

## Version Rules

The system enforces these rules automatically:

### Major Version (X.0.0)
- Minor and patch **must** reset to 0
- ✅ v1.5.3 → v2.0.0
- ❌ v1.5.3 → v2.1.0 (invalid)

### Minor Version (x.Y.0)
- Patch **must** reset to 0
- ✅ v1.5.3 → v1.6.0
- ❌ v1.5.3 → v1.6.1 (invalid)

### Patch Version (x.y.Z)
- Automatic on every push to main
- Can be any increment

## When to Release

### Major Version (Breaking Changes)
- Removing features
- Changing protocol/API incompatibly
- Changing default behavior that breaks existing setups
- Renaming command-line options

**Example**: Changing WebSocket protocol format

### Minor Version (New Features)
- Adding new monitoring capabilities
- Adding new command-line options
- Adding new API endpoints
- Improving features without breaking changes

**Example**: Adding PostgreSQL monitoring support

### Patch Version (Bug Fixes)
- Fixing bugs
- Performance improvements
- Documentation updates
- Security patches

**Example**: Fixing memory leak (automatic)

## Troubleshooting

### Workflow Fails

**Version validation error**:
- Check that you're following SemVer rules
- Major bumps must reset minor/patch to 0
- Minor bumps must reset patch to 0

**Tag already exists**:
- Someone may have already created this version
- Check existing tags: `git tag -l`
- Delete if needed: `git push origin :refs/tags/vX.Y.Z`

### Need to Undo a Release

```bash
# Delete tag locally
git tag -d vX.Y.Z

# Delete tag remotely
git push origin :refs/tags/vX.Y.Z

# Delete release on GitHub
# Go to Releases → Click release → Delete release
```

Then create the correct version.

## Best Practices

1. **Test before releasing**
   - Ensure main branch is stable
   - All CI checks should pass
   - Test locally if possible

2. **Coordinate major releases**
   - Announce breaking changes in advance
   - Update documentation
   - Notify users

3. **Review changelog**
   - Auto-generated from commits
   - Edit release notes if needed
   - Add migration guides for major versions

4. **Let patches auto-release**
   - Don't manually create patch versions
   - They happen automatically on every merge

5. **Use semantic commit messages**
   - They appear in the changelog
   - Be descriptive: "Fix memory leak in CPU monitor"
   - Not: "Fix bug"

## Quick Reference

| Action | Version Change | How to Do It |
|--------|---------------|--------------|
| Bug fix | v1.2.3 → v1.2.4 | Push to main (automatic) |
| New feature | v1.2.3 → v1.3.0 | Actions → Manual Release → minor |
| Breaking change | v1.2.3 → v2.0.0 | Actions → Manual Release → major |

## Questions?

- Check [VERSIONING.md](../VERSIONING.md) for detailed version rules
- Review [GitHub Actions](https://github.com/nerdpitchcloud/blinky/actions) for workflow status
- Check [Releases](https://github.com/nerdpitchcloud/blinky/releases) for published versions
