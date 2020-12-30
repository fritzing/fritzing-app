# Need to discard STDERR so get path to NULL device
win32 {
    NULL_DEVICE = NUL # Windows doesn't have /dev/null but has NUL
} else {
    NULL_DEVICE = /dev/null
}

# Need to call git with manually specified paths to repository
BASE_GIT_COMMAND = git --git-dir $$PWD/../.git --work-tree $$PWD/..

$$(CI) {
    # When running on travis ci, the tag is created *after* the build was successful.
    # So we can not use 'git describe', but we generate a string that should look exactly
    # like what git describe would deliver
    GIT_VERSION = CD-$$(TRAVIS_BUILD_NUMBER)-0-$$system($$BASE_GIT_COMMAND rev-parse --short HEAD)
} else {
    GIT_VERSION = $$system($$BASE_GIT_COMMAND describe --tags 2> $$NULL_DEVICE)
}

GIT_DATE = $$system($$BASE_GIT_COMMAND show --no-patch --no-notes --pretty='%cd' HEAD --date=iso-strict 2> $$NULL_DEVICE)

# Here we process the build date and time
win32 {
    # Try to squeeze something ISO-8601-ish out of windows
    BUILD_DATE = $$system( powershell (Get-Date -Format "o") )
}

macx {
    # MacOS uses BSD date rather than GNU date. 
    BUILD_DATE = $$system( date +%Y-%m-%dT%H:%M:%S%z )
}

!win32:!macx {
    BUILD_DATE = $$system( date --iso-8601=seconds )
}

DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"
DEFINES += GIT_DATE=\\\"$$GIT_DATE\\\"
DEFINES += BUILD_DATE=\\\"$$BUILD_DATE\\\"
