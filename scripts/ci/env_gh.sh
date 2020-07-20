#!/bin/bash -e

if [[ $GITHUB_REF == *"refs/heads"* ]]
then 
    TMP_CI_BRANCH=${GITHUB_REF#refs/heads/};
elif [[ $GITHUB_REF == *"refs/tags"* ]]
then 
    TMP_CI_TAG=${GITHUB_REF#refs/tags/}
    TMP_CI_BRANCH=$TMP_CI_TAG
else
    ${TMP_CI_BRANCH:=""}
    ${TMP_CI_TAG:=""}
fi

TMP_CI_COMMIT_BEFORE=$(jq --raw-output .before "$GITHUB_EVENT_PATH")
TMP_CI_COMMIT_AFTER=$(jq --raw-output .after "$GITHUB_EVENT_PATH")

CI_PULL_REQUEST=${CI_PULL_REQUEST:=$(jq --raw-output .pull_request.number "$GITHUB_EVENT_PATH")}

export CI_BUILD_DIR=${CI_BUILD_DIR:=$GITHUB_WORKSPACE}
export CI_COMMIT=${CI_COMMIT:=$GITHUB_SHA}
export CI_BRANCH=${CI_BRANCH:=$TMP_CI_BRANCH}
export CI_TAG=${CI_TAG:=$TMP_CI_TAG}
export CI_SECURE_ENV_VARS=${CI_SECURE_ENV_VARS:=false} # TODO
export CI_PULL_REQUEST=${CI_PULL_REQUEST:=false}
export CI_PULL_REQUEST_BRANCH=${CI_PULL_REQUEST_BRANCH:=$TRAVIS_PULL_REQUEST_BRANCH} # TODO
export CI_COMMIT_RANGE=${CI_COMMIT_RANGE:="$TMP_CI_COMMIT_BEFORE...$TMP_CI_COMMIT_AFTER"}
export CI_REPO_SLUG=${CI_REPO_SLUG:=$GITHUB_REPOSITORY}
export CI_UPLOAD_ARTIFACT_ID=${CI_UPLOAD_ARTIFACT_ID:=$( [[ ${CI_PULL_REQUEST} =~ false ]] && echo ${CI_TAG} || echo ${CI_PULL_REQUEST} )}

# make sure ::set-env is on newlines 
echo ""
echo "::set-env name=CI_BUILD_DIR::$CI_BUILD_DIR"
echo "::set-env name=CI_COMMIT::$CI_COMMIT"
echo "::set-env name=CI_BRANCH::$CI_BRANCH"
echo "::set-env name=CI_TAG::$CI_TAG"
echo "::set-env name=CI_SECURE_ENV_VARS::$CI_SECURE_ENV_VARS"
echo "::set-env name=CI_PULL_REQUEST::$CI_PULL_REQUEST"
echo "::set-env name=CI_PULL_REQUEST_BRANCH::$CI_PULL_REQUEST_BRANCH"
echo "::set-env name=CI_COMMIT_RANGE::$CI_COMMIT_RANGE"
echo "::set-env name=CI_REPO_SLUG::$CI_REPO_SLUG"
echo "::set-env name=CI_UPLOAD_ARTIFACT_ID::$CI_UPLOAD_ARTIFACT_ID"
echo ""
