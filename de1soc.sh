#!/bin/bash

# Default configurations
SSH_USER="${SSH_USER:-root}"
TARGET_ADDR="${TARGET_ADDR:-192.168.0.2}"
DEPLOY_PATH="${DEPLOY_PATH:-/root}"

# Check arguments
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <deploy|run|kill> <program> [args...]"
    exit 1
fi

ACTION="$1"
PROGRAM="$2"
shift 2
ARGS="$@"

BASENAME=$(basename "$PROGRAM")
REMOTE_PATH="$DEPLOY_PATH/$BASENAME"

deploy() {
    echo "Deploying $PROGRAM to $SSH_USER@$TARGET_ADDR:$REMOTE_PATH"
    scp "$PROGRAM" "$SSH_USER@$TARGET_ADDR:$REMOTE_PATH"
}

case "$ACTION" in
    deploy)
        deploy
        ;;
    run)
        deploy && ssh -t "$SSH_USER@$TARGET_ADDR" "chmod +x '$REMOTE_PATH' && '$REMOTE_PATH' $ARGS"
        ;;
    kill)
        echo "Killing $BASENAME on $SSH_USER@$TARGET_ADDR"
        ssh "$SSH_USER@$TARGET_ADDR" "pidof '$BASENAME' && pkill -f '$BASENAME' || echo 'Process not running'"
        ;;
    *)
        echo "Invalid action: $ACTION"
        echo "Usage: $0 <deploy|run|kill> <program> [args...]"
        exit 1
        ;;
esac