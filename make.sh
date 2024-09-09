#!/bin/bash

# Clone the repository
clone_repo() {
    if [ ! -d "portal_turret_webpage" ]; then
        git clone https://github.com/joranderaaff/portal_turret_webpage.git
    fi
}

# Install npm dependencies
install_dependencies() {
    if [ ! -d "portal_turret_webpage/node_modules" ]; then
        clone_repo
        cd portal_turret_webpage
        npm install
        cd ..
    fi
}

# Build the project
build_project() {
    install_dependencies
    cd portal_turret_webpage
    npm run build
    cp dist_compiled/index.html.gz.h ../generated/
    cd ..
}

# Clean up the project
distclean() {
    rm -rf portal_turret_webpage
}

# Extract audio from portal 2 data
audio() {
    PORTAL2_DIR="c:/Program Files (x86)/Steam/steamapps/common/Portal 2"
    cd audio
    if [ ! -e ffmpeg ]; then
      curl -L https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip -o ffmpeg.zip
      unzip ffmpeg.zip
      mv ffmpeg-* ffmpeg
      rm ffmpeg.zip
    fi
    rm -rf out
    mkdir -p out
    export PATH=$PATH:`pwd`/ffmpeg/bin
    if [ "$1" = "all" ]; then
      python extract.py "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
      mv out english
      for i in german french spanish russian; do
        mkdir out
        echo will extract $i language
        read
        python extract.py "${PORTAL2_DIR}/portal2_$i/pak01_dir.vpk" "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
        mv out $i
      done
    elif [ "$1" == "" -o "$1" == "english" ]; then
      python extract.py "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
    else
      python extract.py "${PORTAL2_DIR}/portal2_$1/pak01_dir.vpk" "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
    fi
}

# Download littlefs partition
fs_get() {
    rm -rf fs
    pip install littlefs-python
    OFFSET=`cat partitions.csv  | grep spiffs | awk -F, '{print $4}'`
    SIZE=`cat partitions.csv | grep spiffs | awk -F, '{print $5}'`
    esptool.py read_flash $OFFSET $SIZE fs.bin
    littlefs-python extract fs.bin fs --block-size=4096
    rm fs.bin
}

# Main function to control the flow
main() {
    case "$1" in
        distclean)
            distclean
            ;;
        audio)
            shift
            audio $*
            ;;
        fs_get)
            fs_get
            ;;
        *)
            build_project
            ;;
    esac
}

# Execute main function with passed arguments
main "$@"