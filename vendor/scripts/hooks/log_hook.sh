#!/bin/sh
# Colored logging utility for git hooks

LOG_DIR=".git/hook-logs"
mkdir -p "$LOG_DIR"

# ANSI color codes
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

timestamp() {
	date +"%Y-%m-%d %H:%M:%S"
}

log_hr() {
	/bin/echo -e "${BOLD}--------------------------------------------------${NC}" | tee -a "$LOG_DIR/hook.log"
}

log_broadcast() {
	title="$1"
	log_hr
	/bin/echo -e "${GREEN}${BOLD}${title} [$(timestamp)]${NC}" | tee -a "$LOG_DIR/hook.log"
	log_hr
}

log_info() {
	/bin/echo -e "  ${BLUE}[INFO]${NC} $*" | tee -a "$LOG_DIR/hook.log"
}

log_error() {
	/bin/echo -e "  ${RED}[ERROR]${NC} $*" | tee -a "$LOG_DIR/hook.log" >&2
}

log_warn() {
	/bin/echo -e "  ${YELLOW}[WARN]${NC} $*" | tee -a "$LOG_DIR/hook.log"
}

log_success() {
	/bin/echo -e "  ${GREEN}[SUCCESS]${NC} $*" | tee -a "$LOG_DIR/hook.log"
}

log_hook() {
	hook="$1"
	shift
	/bin/echo -e "  ${GREEN}[$hook]${NC} $*" | tee -a "$LOG_DIR/hook.log"
}

log_example() {
	/bin/echo -e "  ${BOLD}${GREEN}[EXAMPLE]${NC} $*" | tee -a "$LOG_DIR/hook.log"
}
