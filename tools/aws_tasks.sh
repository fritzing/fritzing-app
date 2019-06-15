#!/usr/bin/env bash

# This script is used on main repo via travis.
# It enables travis jobs to store builds on S3 for release packaging

mkdir -p ~/.aws

cat > ~/.aws/credentials << EOL
[default]
aws_access_key_id = ${AWS_ACCESS_KEY_ID}
aws_secret_access_key = ${AWS_SECRET_ACCESS_KEY}
EOL

# set up awscli packages
pip install --user awscli
mkdir -p ~/$TRAVIS_BUILD_NUMBER
aws s3 sync s3://fritzing/$TRAVIS_BUILD_NUMBER ~/$TRAVIS_BUILD_NUMBER
