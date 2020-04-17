FROM ubuntu:19.10
MAINTAINER <genc.5@osu.edu>

ENV HOME /
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
 && apt-get install -y g++ cmake google-perftools libgoogle-perftools-dev less nano vim python3 python3-pip texlive texlive-latex-extra texlive-font-utils latexmk time

COPY ./exparser /exparser
COPY ./misc/parse-results.sh /bin/parse-results
COPY ./misc/matplotlibrc /matplotlibrc

RUN cd /exparser && pip3 install -r requirements.txt && pip3 install
RUN mkdir /results /Crafty
RUN cd /bin && \
    ln -s /Crafty/compare.sh && \
    ln -s /exparser/scripts/Crafty.py && \
    chmod +x Crafty.py
