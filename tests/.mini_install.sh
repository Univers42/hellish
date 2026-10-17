say(){ printf '\033[1;36mhellish:\033[0m %s\n' "$*"; }
say 'Installing opencode version 1.2.3 for x86_64'
payload=$(printf '%4067s' '')
payload="\${payload}Manually add this path to PATH"
say "$payload"
