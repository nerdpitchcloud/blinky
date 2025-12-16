#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$REPO_ROOT"

show_usage() {
    echo "Usage: $0 [major|minor|patch]"
    echo ""
    echo "Create a new release version tag following SemVer rules:"
    echo "  major - Increment major version (resets minor and patch to 0)"
    echo "  minor - Increment minor version (resets patch to 0)"
    echo "  patch - Increment patch version"
    echo ""
    echo "Examples:"
    echo "  $0 major  # 1.2.3 -> 2.0.0"
    echo "  $0 minor  # 1.2.3 -> 1.3.0"
    echo "  $0 patch  # 1.2.3 -> 1.2.4"
    echo ""
    echo "Note: Patch versions are automatically created on every push to main."
    echo "      Use this script only for major or minor version bumps."
    exit 1
}

if [ $# -ne 1 ]; then
    show_usage
fi

TYPE=$1

if [ "$TYPE" != "major" ] && [ "$TYPE" != "minor" ] && [ "$TYPE" != "patch" ]; then
    echo "Error: Invalid version type '$TYPE'"
    show_usage
fi

LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
echo "Current version: $LATEST_TAG"

VERSION=${LATEST_TAG#v}
MAJOR=$(echo $VERSION | cut -d. -f1)
MINOR=$(echo $VERSION | cut -d. -f2)
PATCH=$(echo $VERSION | cut -d. -f3)

case $TYPE in
    major)
        NEW_MAJOR=$((MAJOR + 1))
        NEW_MINOR=0
        NEW_PATCH=0
        ;;
    minor)
        NEW_MAJOR=$MAJOR
        NEW_MINOR=$((MINOR + 1))
        NEW_PATCH=0
        ;;
    patch)
        NEW_MAJOR=$MAJOR
        NEW_MINOR=$MINOR
        NEW_PATCH=$((PATCH + 1))
        ;;
esac

NEW_VERSION="v$NEW_MAJOR.$NEW_MINOR.$NEW_PATCH"

echo "New version: $NEW_VERSION"
echo ""

if git rev-parse "$NEW_VERSION" >/dev/null 2>&1; then
    echo "Error: Tag $NEW_VERSION already exists"
    exit 1
fi

read -p "Create and push tag $NEW_VERSION? (y/N) " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Cancelled"
    exit 0
fi

echo "Creating tag $NEW_VERSION..."
git tag -a "$NEW_VERSION" -m "Release $NEW_VERSION"

echo "Pushing tag to origin..."
git push origin "$NEW_VERSION"

echo ""
echo "Done! Tag $NEW_VERSION has been created and pushed."
echo "GitHub Actions will now build and create the release."
echo ""
echo "View the release at:"
echo "https://github.com/nerdpitchcloud/blinky/releases/tag/$NEW_VERSION"
