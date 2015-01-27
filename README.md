# gst-buzztrax

**This module has been discontinued and merged into buzztrax!**

## quick info
Please turn you browser to http://www.buzztrax.org to learn what this project
is about. Buzztrax is free software and distributed under the LGPL.

## build status
We are running continuous integration on drone.io

[![Build Status](https://drone.io/github.com/Buzztrax/gst-buzztrax/status.png)](https://drone.io/github.com/Buzztrax/gst-buzztrax/latest)

## intro
This module contains experimental code that extends gstreamer. The library will
never be ABI stable. if you are interested in any code here, please help to
mature it in order to get it into upstream.
The extensions are quite likely to be required to build a git version of
buzztrax.

## installing locally
You can install this module locally too.
Use following option for ./autogen.sh or ./configure

    --prefix=$HOME/buzztrax

When installing the package to e.g. $HOME/buzztrax remember to set these
environment options:

    # libraries:
    export LD_LIBRARY_PATH=$HOME/buzztrax/lib:$LD_LIBRARY_PATH
    #devhelp:
    export DEVHELP_SEARCH_PATH="$DEVHELP_SEARCH_PATH:$HOME/buzztrax/share/gtk-doc/html"
    #pkg-config:
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$HOME/buzztrax/lib/pkgconfig"
    #gstreamer
    export GST_PLUGIN_PATH_1.0="$HOME/buzztrax/lib/gstreamer-1.0"

## installing in /usr/local =
The default value for the --prefix configure option is /usr/local. Also in that
case the variables mentioned in the last example need to be exported.

## running unit tests
run all the tests:

    make check

