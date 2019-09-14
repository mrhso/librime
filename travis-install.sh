#!/bin/bash

cd "$(dirname "$0")"

if [[ -n "$TRAVIS_OS_NAME" ]]; then
  bash "./travis-install-${TRAVIS_OS_NAME}.sh"
fi

if [[ -n "${RIME_PLUGINS}" ]]; then
    # intentionally unquoted: ${RIME_PLUGINS} is a space separated list of slugs
    bash ./install-plugins.sh ${RIME_PLUGINS}
    for plugin_dir in plugins/*; do
        if [[ -e "${plugin_dir}/travis-install.sh" ]]; then
	    (cd "${plugin_dir}"; bash ./travis-install.sh)
        fi
    done
fi
