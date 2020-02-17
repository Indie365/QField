#!/bin/bash

echo "travis_fold:start:tx-pull"


if [[ ${TRAVIS_BRANCH} = master ]]; then
  tx pull --all --force
else
  tx pull --all --force --branch
fi

for x in android/res/values-*_*;do mv $x $(echo $x | sed -e 's/_/-r/') ;done

lrelease QField.pro

echo "travis_fold:end:tx-pull"

