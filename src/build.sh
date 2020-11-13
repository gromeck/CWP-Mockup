#!/bin/bash

aclocal \
&& autoheader \
&& automake --gnu --add-missing --copy \
&& autoconf \
&& ./configure $* \
&& make clean \
&& make
