#!/bin/bash

# Root directory for the rename
ROOT_DIR="."

# Old and new names
OLD="mad-hatter"
NEW="rinox"

# Find all text files except .sh files, and replace occurrences in-place
grep -rl "$OLD" "$ROOT_DIR" \
    --exclude-dir=.git \
    --exclude="*.sh" | while read -r file; do
    echo "Updating $file"
    sed -i "s/${OLD}/${NEW}/g" "$file"
done

echo "Done."
