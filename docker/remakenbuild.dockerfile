FROM remaken/packager:v1.0
# base image contains latest ubuntu LTS

LABEL description="Docker image to build remaken"
LABEL maintainer="SFT Team<sft@b-com.com>"

ENV pkgVersion 1.69.0
ENV QT_SELECT qt5

# packages needed to build remaken
RUN set -ex && \
   apt-get update && apt-get install -y \
     build-essential \
     cmake \
     openssl \
     zlib1g \
     zlib1g-dev \
     python-pip \
     python3-venv \
     python3-pip \
     python3-setuptools \
     git \
     && rm -rf /var/lib/apt/lists/*

RUN set -ex && \
     pip install wheel \
     pip install conan

CMD ["bash"]
