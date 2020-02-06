FROM ubuntu:latest
# base image contains latest ubuntu LTS
# the context is the sft-tools git repository root folder

LABEL description="Docker image to package a build for remaken"
LABEL maintainer="SFT Team<sft@b-com.com>"

# packages needed to build remaken
RUN set -ex && \
   apt-get update && apt-get install -y \
     perl \
     libterm-readkey-perl \
     libgetopt-long-descriptive-perl \
     git \
     && rm -rf /var/lib/apt/lists/*

# scripts copied from sft-tools git repository
COPY package-compress.sh /usr/local/bin/
COPY remaken-packager.pl /usr/local/bin/

CMD ["bash"]
