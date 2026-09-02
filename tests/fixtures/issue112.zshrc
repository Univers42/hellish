# Habilitar el módulo de información de control de versiones (Git)
autoload -Uz vcs_info
precmd() { vcs_info }

# Configurar el formato visual de Git (rama + estado de cambios)
zstyle ':vcs_info:git:*' enable git
zstyle ':vcs_info:git:*' check-for-changes true
zstyle ':vcs_info:git:*' stagedstr '%F{green}+%f'
zstyle ':vcs_info:git:*' unstagedstr '%F{red}*%f'
zstyle ':vcs_info:git:*' formats '%F{242}on%f %F{magenta}󰘬 %b%c%u%f '
zstyle ':vcs_info:git:*' actionformats '%F{242}on%f %F{magenta}󰘬 %b%f%%F{yellow}|%a%f '

# Prompt principal de 2 líneas
PROMPT='%F{cyan}%~%f ${vcs_info_msg_0_}
%(?.%F{green}❯%f.%F{red}❯%f) '

# Prompt derecho con icono de usuario y hora discreta
RPROMPT='%F{242}%n@%m %T%f'
