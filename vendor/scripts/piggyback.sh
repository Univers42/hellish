#/bin/bash

# For example, in .git/hooks/post-commit:

git pull origin develop
echo "$(date): $(git log -1 --pretty=%B)" >> monitor_log.txt