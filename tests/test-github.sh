 #!/bin/bash
 ALTERNATE_REPO_URL="https://..."
 remaken --action install -r  /Users/ltouraine/tmp/remaken_alt_test -s linux -t github -l artifactory -u $ALTERNATE_REPO_URL --cpp-std 17 -c debug -f samples/packagedependencies-github.txt
