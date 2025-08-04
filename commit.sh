#!/ bin / bash

message="$1"

for file in $(git status --porcelain | awk '{print $2}');
do
git status
        clang -
    format - i "../$file" git add "../$file" git status done

                 git commit -
    m "$message" git push